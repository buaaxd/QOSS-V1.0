#include "SourceModeGeneration.h"

calculation::SourceModeGeneration::SourceModeGeneration(
	int _SourceKind, int _SourceType, int _Rotation, int _m, int _n,
	double _Frequency, double _Radius, double _Height, double _Width,
	int _N) :
	SourceKind(_SourceKind), SourceType(_SourceType), Rotation(_Rotation),
	m(_m), n(_n), Frequency(_Frequency), Radius(_Radius),
	Height(_Height), Width(_Width),
	N(_N)
{
	ds = _Radius * 2.0 / (_N - 1);//采样间距,默认为圆波导类型

	EX.resize(_N); HX.resize(_N);
	EY.resize(_N); HY.resize(_N);
	EZ.resize(_N); HZ.resize(_N);
	for (int i = 0; i < _N; i++)
	{
		EX[i].resize(_N); HX[i].resize(_N);
		EY[i].resize(_N); HY[i].resize(_N);
		EZ[i].resize(_N); HZ[i].resize(_N);
	}//生成6个分量的矩阵尺寸：N*N
}//构造函数定义

 //析构函数还需要定义吗？

 //设置圆波导的参数
void calculation::SourceModeGeneration::SetSource_Circular(int sourcekind, int sourcetype,
	int rotation, int m0, int n0, double frequency, double radius)
{
	SourceKind = sourcekind;//只能是1或者2
	SourceType = sourcetype;
	Rotation = rotation;
	m = m0;
	n = n0;
	Frequency = frequency;
	Radius = radius;
	ds = 2.0*Radius / (N - 1);
	Height = 0;
	Width = 0;
}

void calculation::SourceModeGeneration::SetSource_Rectangle(int sourcekind, int sourcetype,
	int m0, int n0, double frequency, double height, double width)

{
	SourceKind = sourcekind;//只能是3
	SourceType = sourcetype;
	m = m0;
	n = n0;
	Frequency = frequency;
	Radius = 0;
	Height = height;
	Width = width;
	ds = Height / (N - 1);
}

void calculation::SourceModeGeneration::SetOutputProperty(int n1)
{
	N = n1;//采样点
	if (SourceKind == 1 || SourceKind == 2)//如果是圆波导的话
		ds = Radius * 2.0 / (N - 1);//圆波导的采样间距
	else  ds = Height / (N - 1);//矩形波导的采样间距，以长边为准

	EX.resize(N); HX.resize(N);
	EY.resize(N); HY.resize(N);
	EZ.resize(N); HZ.resize(N);
	for (int i = 0; i < N; i++)
	{
		EX[i].resize(N); HX[i].resize(N);
		EY[i].resize(N); HY[i].resize(N);
		EZ[i].resize(N); HZ[i].resize(N);
	}//生成6个分量的矩阵尺寸：N*N

	 //20171030：需要李赟检查一下写的对不对
}


bool calculation::SourceModeGeneration::SetWaveguideMode(int m0, int n0)
{
	if ((m0 < 0) || (n0 < 0)) return false;
	m = m0;
	n = n0;
	return true;
}

//计算并输出圆波导模式场分布
bool calculation::SourceModeGeneration::FieldCalculation_Circular()
{
	//开始计算场分布
	double Lamda = C_Speed / Frequency;
	double K0 = 2 * Pi / Lamda;
	double Omega = 2 * Pi * Frequency;

	//之前利用取较小delta根据公式求导的方式精度不达标，
	//因此听取白老师的意见，对bessel函数的级数展开式进行求导，再利用夹逼法得到导数零点值
	//20171026金铭学长给出快速计算导数根公式，采用

	const int MaxLoopNumber = 10000;//最大循环次数
	const double step0 = 0.1; //粗扫的步进
	const double step1 = step0 / MaxLoopNumber;//第一次细扫的步进

	int n0 = 0;//第n0个导数根的初始化
	int i0 = 0;//找到了第n个导数根后的区间标记
	for (int i = 1; i <= MaxLoopNumber; i++)//特意不从0开始
	{
		if ((D1_J(m, i*step0) >= 0.0 && D1_J(m, (i + 1)*step0) <= 0.0)
			|| (D1_J(m, i*step0) <= 0.0 && D1_J(m, (i + 1)*step0) >= 0.0))
		{
			n0 = n0 + 1;//开始计数
			i0 = i;//记下该区间
		}
		if (n0 == n) break;//找到第n个导数根后直接跳出循环，此时导数根落在[i0*ds，(i0+1)*ds]区间上

	}

	//开始第一次细扫[i0*step0，(i0+1)*step0]区间,间隔更小
	int j0 = 0;//找到了第n个导数根后的区间标记
	for (int j = 0; j <= MaxLoopNumber; j++)//从0开始
	{

		if ((D1_J(m, j*step1 + i0*step0) >= 0.0 && D1_J(m, (j + 1)*step1 + i0*step0) <= 0.0)
			|| (D1_J(m, j*step1 + i0*step0) <= 0.0 && D1_J(m, (j + 1)*step1 + i0*step0) >= 0.0))
		{
			j0 = j;//找到了这个区间，并记下该区间下标
			break;//找到第n个导数根后直接跳出循环，此时导数根落在[i*ds，(i+1)*ds]区间上
		}
	}

	double Xmn = 0.5*((j0*step1 + i0*step0) + ((j0 + 1)*step1 + i0*step0));
	//20171026已经验证，完全符合MathCad计算结果

	double Kr = Xmn / Radius;
	if (Kr > K0) return false;//截止

	double Kz = pow((K0*K0 - Kr*Kr), 0.5);
	double Lamda_c = 2.0 * Pi / Kr;
	double Beta = 2.0 * Pi*Frequency / C_Speed*pow((1.0 - Lamda*Lamda / Lamda_c / Lamda_c), 0.5);

	//下面开始编写输出场分布
	vector <double> X(N);//X坐标，点数为N
	vector <double> Y(N);//Y坐标，点数为N
	vector <vector <double>> R(N), JUDGE(N), PHI(N);
	vector <vector <complex <double>>> ER(N), HR(N), EPhi(N), HPhi(N);//复数矩阵
	for (int i = 0; i < N; i++)
	{
		R[i].resize(N); JUDGE[i].resize(N); PHI[i].resize(N);
		ER[i].resize(N); HR[i].resize(N); EPhi[i].resize(N); HPhi[i].resize(N);
	}//生成R以及相关矩阵的尺寸：N*N

	for (int i = 0; i < N; i++)
	{
		X[i] = ds*(i - (N - 1.0) / 2.0);
		for (int j = 0; j < N; j++)
		{
			Y[j] = ds*(j - (N - 1.0) / 2.0);
			R[i][j] = pow((X[i] * X[i] + Y[j] * Y[j]), 0.5);

			JUDGE[i][j] = 0.0;
			if (R[i][j]<Radius) JUDGE[i][j] = 1.0;//点在半径范围内JUDGE就为1，否则为0

			if ((X[i] == 0) && (Y[j] == 0)) PHI[i][j] = 0.0;
			else PHI[i][j] = atan(Y[j] / X[i]);//各点对应的夹角phi
											   //atan输出范围-pi/2到pi/2

			EZ[i][j] = 0;//TE模式，Ez为0

			if (JUDGE[i][j] != 0.0)//如果在波导内部
			{
				if (Rotation != 1)//非左旋
				{
					ER[i][j] = E_r(Kz, Kr, m, R[i][j], PHI[i][j], Omega, Beta);
					HR[i][j] = H_r(Kz, Kr, m, R[i][j], PHI[i][j]);
					EPhi[i][j] = E_phi(Kz, Kr, m, R[i][j], PHI[i][j], Omega, Beta);
					HPhi[i][j] = H_phi(Kz, Kr, m, R[i][j], PHI[i][j]);
					HZ[i][j] = H_z(Kr, m, R[i][j], PHI[i][j]);
				}
				else //如果是左旋
				{
					ER[i][j] = E_r_LeftRo(Kz, Kr, m, R[i][j], PHI[i][j], Omega, Beta);
					HR[i][j] = H_r_LeftRo(Kz, Kr, m, R[i][j], PHI[i][j]);
					EPhi[i][j] = E_phi_LeftRo(Kz, Kr, m, R[i][j], PHI[i][j], Omega, Beta);
					HPhi[i][j] = H_phi_LeftRo(Kz, Kr, m, R[i][j], PHI[i][j]);
					HZ[i][j] = H_z_LeftRo(Kr, m, R[i][j], PHI[i][j]);
				}

				EX[i][j] = cos(PHI[i][j])*ER[i][j] - sin(PHI[i][j])*EPhi[i][j];
				HX[i][j] = cos(PHI[i][j])*HR[i][j] - sin(PHI[i][j])*HPhi[i][j];
				EY[i][j] = sin(PHI[i][j])*ER[i][j] + cos(PHI[i][j])*EPhi[i][j];
				HY[i][j] = sin(PHI[i][j])*HR[i][j] + cos(PHI[i][j])*HPhi[i][j];
			}
			else//如果在波导外部
			{
				ER[i][j] = 0; HR[i][j] = 0; EPhi[i][j] = 0; HPhi[i][j] = 0; HZ[i][j] = 0;
				EX[i][j] = 0; HX[i][j] = 0; EY[i][j] = 0; HY[i][j] = 0;
			}//在圆波导范围外场值全为0
		}
	}
	return true;//算完了
}

//计算矩形波导场分布
bool calculation::SourceModeGeneration::FieldCalculation_Rectangle()
{
	double Lamda = C_Speed / Frequency;
	double K0 = 2 * Pi / Lamda;
	double Omega = 2 * Pi * Frequency;
	double Lamda_c = 2.0 / pow((m*m*Pi*Pi / Height / Height + n*n*Pi*Pi / Width / Width), 0.5);
	double Kc = 2.0 * Pi / Lamda_c;

	if (Kc > K0) return false;//Kc大于K0时，波导截止，不用再算下去了

	double Beta = 2.0 * Pi*Frequency / C_Speed*pow((1.0 - Lamda*Lamda / Lamda_c / Lamda_c), 0.5);
	complex <double> a1(0, 1);//定义虚数j

							  //下面开始编写矩形波导输出场分布
	vector <double> X(N);//X坐标，点数为N
	vector <double> Y(N);//Y坐标，点数为N
	vector <vector <double>> JUDGE(N);
	for (int i = 0; i < N; i++)
	{
		JUDGE[i].resize(N);
	}//生成JUDGE以及相关矩阵的尺寸：N*N

	for (int i = 0; i < N; i++)
	{
		X[i] = ds*i;//这里的X与Y的起始点为(0,0)，终点为(a,b)
		for (int j = 0; j < N; j++)
		{
			Y[j] = ds*j;
			JUDGE[i][j] = 0;
			if ((abs((X[i]) - ds*(N - 1.0) / 2.0) < 0.5*Height) &&
				(abs((Y[j] - ds*(N - 1.0) / 2.0)) < 0.5*Width))
				JUDGE[i][j] = 1;//在波导内部的时候JUDGE值为1

			if (JUDGE[i][j] != 0.0)//如果在波导内部
			{
				if (SourceType == 1)//TE模式
				{
					HX[i][j] = a1*Beta / Kc / Kc*(m*Pi / Height)*sin(m*Pi / Height*X[i])*cos(n*Pi / Width*Y[i]);
					HY[i][j] = a1*Beta / Kc / Kc*(n*Pi / Width)*cos(m*Pi / Height*X[i])*sin(n*Pi / Width*Y[i]);
					HZ[i][j] = cos(m*Pi / Height*X[i])*cos(n*Pi / Width*Y[i]);
					EX[i][j] = Omega*Mu0 / Beta*HY[i][j];
					EY[i][j] = -Omega*Mu0 / Beta*HX[i][j];
					EZ[i][j] = 0;
				}
				if (SourceType == 2)//TM模式
				{
					EX[i][j] = -a1*Beta / Kc / Kc*(m*Pi / Height)*cos(m*Pi / Height*X[i])*sin(n*Pi / Width*Y[i]);
					EY[i][j] = -a1*Beta / Kc / Kc*(n*Pi / Width)*sin(m*Pi / Height*X[i])*cos(n*Pi / Width*Y[i]);
					EZ[i][j] = sin(m*Pi / Height*X[i])*sin(n*Pi / Width*Y[i]);
					HX[i][j] = -Omega*Eps0 / Beta*EY[i][j];
					HY[i][j] = Omega*Eps0 / Beta*EX[i][j];
					HZ[i][j] = 0;
				}
			}
			else//如果在矩形波导外部
			{
				EX[i][j] = 0; EY[i][j] = 0; EZ[i][j] = 0;
				HX[i][j] = 0; HY[i][j] = 0; HZ[i][j] = 0;
			}//在矩形波导范围外场值全为0

		}
	}

	return true;//算完了
}

//设置场强归一化幅度值
bool calculation::SourceModeGeneration::SetFieldAmplitude(double K)
{
	if (K <= 0.0) return false;//K不能为负值
	double MAX_EX = 0.0, MAX_EY = 0.0, MAX_EZ = 0.0, MAX = 0.0;//初始化归零
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			if (abs(EX[i][j]) >= MAX_EX) MAX_EX = abs(EX[i][j]);
			if (abs(EY[i][j]) >= MAX_EY) MAX_EY = abs(EY[i][j]);
			if (abs(EZ[i][j]) >= MAX_EZ) MAX_EZ = abs(EZ[i][j]);
		}
	}

	if (MAX_EX >= MAX_EY) MAX = MAX_EX; else MAX = MAX_EY;
	if (MAX_EZ >= MAX) MAX = MAX_EZ;//得到电场场强最大值MAX

	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			EX[i][j] = EX[i][j] / MAX*K; HX[i][j] = HX[i][j] / MAX*K;
			EY[i][j] = EY[i][j] / MAX*K; HY[i][j] = HY[i][j] / MAX*K;
			EZ[i][j] = EZ[i][j] / MAX*K; HZ[i][j] = HZ[i][j] / MAX*K;
		}
	}//至此电场分布最大值为指定值K
	return true;
}

bool calculation::SourceModeGeneration::GetCircularWaveguideProperty(double &phi1,
	double &theta1, double &Rc)
{
	if (SourceKind == 3) return false;//该函数不适用于矩形波导

									  //再次使用上述已经被验证过的求贝塞尔函数一阶导数根的方法
	double Lamda = C_Speed / Frequency;
	double K0 = 2 * Pi / Lamda;
	double Omega = 2 * Pi * Frequency;

	const int MaxLoopNumber = 10000;//最大循环次数
	const double step0 = 0.1; //粗扫的步进
	const double step1 = step0 / MaxLoopNumber;//第一次细扫的步进

	int n0 = 0;//第n0个导数根的初始化
	int i0 = 0;//找到了第n个导数根后的区间标记
	for (int i = 1; i <= MaxLoopNumber; i++)//特意不从0开始
	{
		if ((D1_J(m, i*step0) >= 0.0 && D1_J(m, (i + 1)*step0) <= 0.0)
			|| (D1_J(m, i*step0) <= 0.0 && D1_J(m, (i + 1)*step0) >= 0.0))
		{
			n0 = n0 + 1;//开始计数
			i0 = i;//记下该区间
		}
		if (n0 == n) break;//找到第n个导数根后直接跳出循环，此时导数根落在[i0*ds，(i0+1)*ds]区间上

	}

	//开始第一次细扫[i0*step0，(i0+1)*step0]区间,间隔更小
	int j0 = 0;//找到了第n个导数根后的区间标记
	for (int j = 0; j <= MaxLoopNumber; j++)//从0开始
	{

		if ((D1_J(m, j*step1 + i0*step0) >= 0.0 && D1_J(m, (j + 1)*step1 + i0*step0) <= 0.0)
			|| (D1_J(m, j*step1 + i0*step0) <= 0.0 && D1_J(m, (j + 1)*step1 + i0*step0) >= 0.0))
		{
			j0 = j;//找到了这个区间，并记下该区间下标
			break;//找到第n个导数根后直接跳出循环，此时导数根落在[i*ds，(i+1)*ds]区间上
		}
	}

	double Xmn = 0.5*((j0*step1 + i0*step0) + ((j0 + 1)*step1 + i0*step0));
	//20171026已经验证，完全符合MathCad计算结果

	double Kr = Xmn / Radius;
	if (Kr > K0) return false;//截止

	double Kz = pow((K0*K0 - Kr*Kr), 0.5);
	double Lamda_c = 2.0 * Pi / Kr;
	double Beta = 2.0 * Pi*Frequency / C_Speed*pow((1.0 - Lamda*Lamda / Lamda_c / Lamda_c), 0.5);

	//下面开始计算布里渊角等相关参数
	phi1 = asin(Kr / K0);//phi1是布里渊角
	theta1 = acos(m / Xmn); //theta1是方向矢量在横向的投影与波导切线的夹角
	Rc = Radius*m / Xmn;//Rc是焦散圆的半径
	return true;
}

bool calculation::SourceModeGeneration::GetCircularConverterLauncherTriangle_TE0n(
	Vector3 &P1, Vector3 &P2, Vector3 &P3)
{
	if ((SourceKind != 1) && (SourceType != 1) && (m != 0)) return false;

	P1 = Vector3(-Radius, 0, 0);
	P2 = Vector3(Radius, 0, 0);
	double phi; double theta; double Rc;
	if (GetCircularWaveguideProperty(phi, theta, Rc))
		P3 = Vector3(Radius, 0, 2.0*Radius / (tan(phi)));
	return true;
}

bool calculation::SourceModeGeneration::GetCircularSystemOpticalPath_TE0n_2Mirrors(
	Vector3 & P4, Vector3 & V1, double & Length1, double & theta1,
	Vector3 & V2, double & Length2, double & theta2, Vector3 & V3)
{
	if ((SourceKind != 1) && (SourceType != 1) && (m != 0)) return false;

	P4 = Vector3(Radius, 0, 0);
	double phi; double theta; double Rc;
	if (GetCircularWaveguideProperty(phi, theta, Rc))//让李赟帮我检查一下
		V1 = Vector3(-1, 0, 1 / (tan(phi)));
	Length1 = (1 + 1 * 1.25)*Radius / (sin(phi));
	theta1 = Pi - 2.0*phi;
	V2 = Vector3(1, 0, 1 / (tan(phi)));
	Length2 = 4 * 1.25*Radius / (cos(0.5*theta1));
	theta2 = 0.5*theta1;
	V3 = Vector3(-1, 0, 0);
	return true;
}
//双镜面光路初值设置完成

bool calculation::SourceModeGeneration::GetCircularSystemOpticalPath_TE0n_3Mirrors(
	Vector3 & P4, Vector3 & V1, double & Length1, double & theta1,
	Vector3 & V2, double & Length2, double & theta2,
	Vector3 & V3, double & Length3, double & theta3, Vector3 & V4)
{
	if ((SourceKind != 1) && (SourceType != 1) && (m != 0)) return false;

	P4 = Vector3(Radius, 0, 0);
	double phi; double theta; double Rc;
	if (GetCircularWaveguideProperty(phi, theta, Rc))//让李赟帮我检查一下
		V1 = Vector3(-1, 0, 1 / (tan(phi)));
	Length1 = (1 + 1 * 1.25)*Radius / (sin(phi));
	theta1 = Pi - 2.0*phi;
	V2 = Vector3(1, 0, 1 / (tan(phi)));
	Length2 = 4 * 1.25*Radius / (cos(0.5*theta1));
	theta2 = 0.75*theta1;
	V3 = Vector3(-1, 0, 1 * tan(0.25*theta1));
	Length3 = 7 * 1.25*Radius / (cos(0.25*theta1));
	theta3 = 0.25*theta1;
	V4 = Vector3(1, 0, 0);
	return true;
}
//三镜面光路初值设置完成

bool calculation::SourceModeGeneration::GetCircularSystemOpticalPath_TE0n_4Mirrors(
	Vector3 & P4, Vector3 & V1, double & Length1, double & theta1,
	Vector3 & V2, double & Length2, double & theta2,
	Vector3 & V3, double & Length3, double & theta3,
	Vector3 & V4, double & Length4, double & theta4, Vector3 & V5)
{
	if ((SourceKind != 1) && (SourceType != 1) && (m != 0)) return false;

	P4 = Vector3(Radius, 0, 0);
	double phi; double theta; double Rc;
	if (GetCircularWaveguideProperty(phi, theta, Rc))//让李赟帮我检查一下
		V1 = Vector3(-1, 0, 1 / (tan(phi)));
	Length1 = (1 + 1 * 1.25)*Radius / (sin(phi));
	theta1 = Pi - 2.0*phi;
	V2 = Vector3(1, 0, 1 / (tan(phi)));
	Length2 = 4 * 1.25*Radius / (cos(0.5*theta1));
	theta2 = 0.75*theta1;
	V3 = Vector3(-1, 0, 1 * tan(0.25*theta1));
	Length3 = 7 * 1.25*Radius / (cos(0.25*theta1));
	theta3 = 0.5*theta1;
	V4 = Vector3(1, 0, 1 * tan(0.25*theta1));
	Length4 = 7 * 1.25*Radius / (cos(0.25*theta1));
	theta4 = 0.25*theta1;
	V4 = Vector3(-1, 0, 0);
	return true;
}
//四镜面光路初值设置完成20171116

//生成一群波导内TE模式的射线向量，用于光线追踪
//20171129已测试TE01模式，没问题
//20171206一个点对应两条光线，一条来自焦散圆，一条来自波导壁反射
//高阶模式未测试
bool calculation::SourceModeGeneration::GetRayTracingSource(int phiNum, int cylinderNum,
	                                                        vector <vector <Vector3>> &RayPosition,
	                                                        vector <vector <Vector3>> &RayVector)
{
	if ((SourceKind == 3) || (SourceType == 2))//如果是矩形波导或者是TM模式，不计算直接报错
		return false;
	if ((phiNum <= 0) || (cylinderNum <= 0))//Phi采样点数和圈数都不能小于等于0
		return false;

	//下面开始对Ray的坐标和方向向量矩阵进行resize
	RayPosition.resize(phiNum * 2); RayVector.resize(phiNum * 2);
	for (int i = 0; i < phiNum * 2; i++)
	{
		RayPosition[i].resize(cylinderNum);
		RayVector[i].resize(cylinderNum);
	}

	double phiGap = 2 * Pi / phiNum;//phi采样间隔

	double phi1; double theta1; double Rc1;
	double cylinderGap;
	if (GetCircularWaveguideProperty(phi1, theta1, Rc1))
		cylinderGap = (Radius - Rc1) / (cylinderNum + 1);//圆周的采样间隔

														 //下面开始为两个矩阵赋值
	for (int i = 0; i < phiNum; i++)
	{
		for (int j = 0; j < cylinderNum; j++)
		{
			RayPosition[i][j] = Vector3((Rc1 + cylinderGap*(j + 1))*cos(phiGap*i),
				(Rc1 + cylinderGap*(j + 1))*sin(phiGap*i),
				0);
			RayPosition[phiNum+i][j] = Vector3((Rc1 + cylinderGap*(j + 1))*cos(phiGap*i),
				(Rc1 + cylinderGap*(j + 1))*sin(phiGap*i),
				0);
			//完成坐标矩阵赋值(重复一遍，因为每个点要对应两条光线)

			double theta = asin(Rc1 / (Rc1 + cylinderGap*(j + 1)));//切线与圆心线的夹角

			if (Rotation != 1)//非左旋情况
			{
				RayVector[i][j] = Vector3(sin(phi1)*cos(phiGap*i + theta),
					sin(phi1)*sin(phiGap*i + theta),
					cos(phi1));
				RayVector[phiNum + i][j] = Vector3(sin(phi1)*cos(Pi + phiGap*i - theta),
					sin(phi1)*sin(Pi + phiGap*i - theta),
					cos(phi1));
			}

			if (Rotation == 1)//左旋情况
			{
				RayVector[i][j] = Vector3(sin(phi1)*cos(phiGap*i - theta),
					sin(phi1)*sin(phiGap*i - theta),
					cos(phi1));
				RayVector[phiNum + i][j] = Vector3(sin(phi1)*cos(Pi + phiGap*i + theta),
					sin(phi1)*sin(Pi + phiGap*i + theta),
					cos(phi1));

			}

		}
	}

}

void calculation::SourceModeGeneration::GetEX(vector<vector<complex<double>>>& EX0)
{
	EX0.resize(N);
	for (int i = 0; i < N; i++)
	{
		EX0[i].resize(N);
	}//生成矩阵尺寸：N*N
	 //下面开始赋值
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			EX0[i][j] = EX[i][j];
		}
	}
}

void calculation::SourceModeGeneration::GetHX(vector<vector<complex<double>>>& HX0)
{
	HX0.resize(N);
	for (int i = 0; i < N; i++)
	{
		HX0[i].resize(N);
	}//生成矩阵尺寸：N*N
	 //下面开始赋值
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			HX0[i][j] = HX[i][j];
		}
	}
}

void calculation::SourceModeGeneration::GetEY(vector<vector<complex<double>>>& EY0)
{
	EY0.resize(N);
	for (int i = 0; i < N; i++)
	{
		EY0[i].resize(N);
	}//生成矩阵尺寸：N*N
	 //下面开始赋值
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			EY0[i][j] = EY[i][j];
		}
	}
}

void calculation::SourceModeGeneration::GetHY(vector<vector<complex<double>>>& HY0)
{
	HY0.resize(N);
	for (int i = 0; i < N; i++)
	{
		HY0[i].resize(N);
	}//生成矩阵尺寸：N*N
	 //下面开始赋值
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			HY0[i][j] = HY[i][j];
		}
	}
}

void calculation::SourceModeGeneration::GetEZ(vector<vector<complex<double>>>& EZ0)
{
	EZ0.resize(N);
	for (int i = 0; i < N; i++)
	{
		EZ0[i].resize(N);
	}//生成矩阵尺寸：N*N
	 //下面开始赋值
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			EZ0[i][j] = EZ[i][j];
		}
	}
}

void calculation::SourceModeGeneration::GetHZ(vector<vector<complex<double>>>& HZ0)
{
	HZ0.resize(N);
	for (int i = 0; i < N; i++)
	{
		HZ0[i].resize(N);
	}//生成矩阵尺寸：N*N
	 //下面开始赋值
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			HZ0[i][j] = HZ[i][j];
		}
	}
}
