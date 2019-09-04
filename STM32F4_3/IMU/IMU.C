#include "IMU.h"
#include "Time2.h"
#include "math.h"

_angle _Att = {0};


#define kp 	    9.0f        //�������� ���Ƽ��ټ�/�����ǵ������ٶ� 
#define ki 	    0.008f     //�������� ��������ƫ�������ٶ�
#define dt      IMU_TIM.delta_time_s //�ɼ�����  ��λ s
#define halfT   dt*0.5f
//#define dt      0.005f //�ɼ�����  ��λ s
//#define halfT	dt/2

float q0 = 1, q1 = 0, q2 = 0, q3 = 0;     //quaternion elements representing theestimated orientation
float exInt = 0, eyInt = 0, ezInt = 0;    //scaled integral error  


/*��Ԫ�� ���� ŷ����
 */
void IMU_update(_angle* att,\
					_F32xyz* acc,\
					_F32xyz* gyro,\
					_F32xyz* mag) 
{
	float norm;            
	float hx, hy, hz, bx, bz;            
	float vx, vy, vz, wx, wy, wz; //v*��ǰ��̬��������������������ϵķ���            
	float ex, ey, ez;             // auxiliary variables to reduce number of repeated operations             
	
	if(acc->x * acc->y * acc->z == 0)
		return;

	if(mag->x * mag->y * mag->z == 0)
		return;
	//��̬����ʱ����
	time_check(&IMU_TIM);
	
	
	// normalise the measurements            
	norm = sqrt(acc->x*acc->x + acc->y*acc->y + acc->z*acc->z);             
	acc->x = acc->x / norm;            
	acc->y = acc->y / norm;            
	acc->z = acc->z / norm;            
	norm = sqrt(mag->x*mag->x + mag->y*mag->y + mag->z*mag->z);            
	mag->x = mag->x / norm;           
	mag->y = mag->y / norm;            
	mag->z = mag->z / norm;                       
	// compute reference direction of magnetic field           
	hx = 2*mag->x*(0.5f - q2*q2 - q3*q3) + 
		 2*mag->y*(q1*q2 - q0*q3) + 
		 2*mag->z*(q1*q3 + q0*q2);           
	hy = 2*mag->x*(q1*q2 + q0*q3) + 
		 2*mag->y*(0.5f - q1*q1 - q3*q3) + 
		 2*mag->z*(q2*q3 - q0*q1);           
	hz = 2*mag->x*(q1*q3 - q0*q2) + 
		 2*mag->y*(q2*q3 + q0*q1) + 
		 2*mag->z*(0.5f - q1*q1 - q2*q2);                    
	bx = sqrt((hx*hx) + (hy*hy));            
	bz = hz;           
	// estimated direction of gravity and magnetic field (v and w) //�ο�����nϵת������������bϵ������Ԫ����ʾ�ķ������Ҿ�������м��ǡ�       
	//���������������            
	vx = 2*(q1*q3 - q0*q2);            
	vy = 2*(q0*q1 + q2*q3);            
	vz = 1 - 2*q1*q1 - 2*q2*q2;
//	vz = q0*q0 - q1*q1 - q2*q2 + q3*q3;//�������mag������ʹ��DCM�õ� 
	
	wx = 2*bx*(0.5f - q2*q2 - q3*q3) + 2*bz*(q1*q3 - q0*q2);            
	wy = 2*bx*(q1*q2 - q0*q3) + 2*bz*(q0*q1 + q2*q3);            
	wz = 2*bx*(q0*q2 + q1*q3) + 2*bz*(0.5f - q1*q1 - q2*q2);            
	/* error is sum of cross product between reference direction of fields and direction measured by sensors 
	�����ڼ��ټƲ����ʹ����Ʋ�������Ϊ�����������ټƲ���û������Z��ı����Ի���Ҫͨ��������������Z�ᡣ����ʽ28����
	����Ԫ��������̬��ȫ���������ϻ��ܡ������߰��ⲿ��������ˣ�����ʲô����Ĳ������ļ�����������ġ����㷽���ǹ�ʽ10��
	*/
	ex = (acc->y*vz - acc->z*vy) + (mag->y*wz - mag->z*wy);           
	ey = (acc->z*vx - acc->x*vz) + (mag->z*wx - mag->x*wz);            
	ez = (acc->x*vy - acc->y*vx) + (mag->x*wy - mag->y*wx);                       
	// integral error scaled integral gain             
	exInt = exInt + ex*ki;//* (1.0f / sampleFreq);            
	eyInt = eyInt + ey*ki;//* (1.0f / sampleFreq);            
	ezInt = ezInt + ez*ki;//* (1.0f / sampleFreq);            
	/* adjusted gyroscope measurements//�����PI�󲹳��������ǣ�
	���������Ư�ơ�ͨ������Kp��Ki�������������Կ��Ƽ��ٶȼ����������ǻ�����̬���ٶȡ�����ʽ16�͹�ʽ29�� 
	*/	
	gyro->x = gyro->x + kp*ex + exInt;            
	gyro->y = gyro->y + kp*ey + eyInt;            
	gyro->z = gyro->z + kp*ez + ezInt;                       
	
	// integrate quaternion rate and normalize //һ�����������������Ԫ��            
	q0 = q0 + (-q1*gyro->x - q2*gyro->y - q3*gyro->z)*halfT;            
	q1 = q1 + ( q0*gyro->x + q2*gyro->z - q3*gyro->y)*halfT;            
	q2 = q2 + ( q0*gyro->y - q1*gyro->z + q3*gyro->x)*halfT;            
	q3 = q3 + ( q0*gyro->z + q1*gyro->y - q2*gyro->x)*halfT;                         
	// normalise quaternion           
	norm = sqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);           
	q0 = q0 / norm;           
	q1 = q1 / norm;            
	q2 = q2 / norm;            
	q3 = q3 / norm;
	
	att->pitch =  atan2(2.0f*(q0*q1 + q2*q3),q0*q0 - q1*q1 - q2*q2 + q3*q3)*rad_to_angle;
	att->roll =  asin(2.0f*(q0*q2 - q1*q3))*rad_to_angle;       
    att->yaw = atan2(2 * q1 * q2 + 2 * q0 * q3, -2 * q2*q2 - 2 * q3* q3 + 1)*rad_to_angle;
//	att->yaw = atan2(2 * q1 * q2 + 2 * q0 * q3, -2 * q2*q2 - 2 * q3* q3 + 1)* 57.3; // yaw
//	att->roll  = asin(-2 * q1 * q3 + 2 * q0* q2)* 57.3; // pitch
//	att->pitch = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2* q2 + 1)* 57.3; // roll
	
/*	
--------------------- 
��Ȩ����������ΪCSDN������_Summer__����ԭ�����£���ѭCC 4.0 by-sa��ȨЭ�飬ת���븽��ԭ�ĳ������Ӽ���������
ԭ�����ӣ�https://blog.csdn.net/qq_21842557/article/details/50993809
*/
	
}



void IMUupdate(	_angle* att,\
				_F32xyz* acc,\
				_F32xyz* gyro,\
				_F32xyz* mag)
{
	float norm;
	float hx, hy, hz, bx, bz;
	float wx, wy, wz;
	float vx, vy, vz;
	float ex, ey, ez;

	// �Ȱ���Щ�õõ���ֵ���
	float q0q0 = q0*q0;
	float q0q1 = q0*q1;
	float q0q2 = q0*q2;
	float q0q3 = q0*q3;
	float q1q1 = q1*q1;
	float q1q2 = q1*q2;
	float q1q3 = q1*q3;
	float q2q2 = q2*q2;
	float q2q3 = q2*q3;
	float q3q3 = q3*q3;

	if(acc->x * acc->y * acc->z == 0)
		return;

	if(mag->x * mag->y * mag->z == 0)
		return;
	//��̬����ʱ����
	time_check(&IMU_TIM);

	norm = sqrt(acc->x * acc->x + acc->y * acc->y + acc->z * acc->z);       //acc���ݹ�һ��


	acc->x = acc->x / norm;
	acc->y = acc->y / norm;
	acc->z = acc->z / norm;

	norm = sqrt(mag->x * mag->x + mag->y * mag->y + mag->z * mag->z);       //mag���ݹ�һ��
 
	mag->x = mag->x / norm;
	mag->y = mag->y / norm;
	mag->z = mag->z / norm;

	//  mx = 0;
	//  my = 0;
	//  mz = 0;

	hx = 2 * mag->x * (0.5f - q2q2 - q3q3) + 2 * mag->y * (q1q2 - q0q3) + 2 * mag->z * (q1q3 + q0q2);  
	hy = 2 * mag->x * (q1q2 + q0q3) + 2 * mag->y * (0.5f - q1q1 - q3q3) + 2 * mag->z * (q2q3 - q0q1);  
	hz = 2 * mag->x * (q1q3 - q0q2) + 2 * mag->y * (q2q3 + q0q1) + 2 * mag->z * (0.5f - q1q1 -q2q2);          
	bx = sqrt((hx*hx) + (hy*hy));  
	bz = hz;

	// estimated direction of gravity and flux (v and w)              �����������������/��Ǩ
	vx = 2*(q1q3 - q0q2);                                             //��Ԫ����xyz�ı�ʾ
	vy = 2*(q0q1 + q2q3);
	vz = q0q0 - q1q1 - q2q2 + q3q3 ;

	wx = 2 * bx * (0.5f - q2q2 - q3q3) + 2 * bz * (q1q3 - q0q2);  
	wy = 2 * bx * (q1q2 - q0q3) + 2 * bz * (q0q1 + q2q3);  
	wz = 2 * bx * (q0q2 + q1q3) + 2 * bz * (0.5f - q1q1 - q2q2); 

	// error is sum of cross product between reference direction of fields and direction measured by sensors
	//  ex = (ay*vz - az*vy) ;                                               //�������������õ���־������
	//  ey = (az*vx - ax*vz) ;
	//  ez = (ax*vy - ay*vx) ;

	ex = (acc->y*vz - acc->z*vy) + (mag->y*wz - mag->z*wy);  
	ey = (acc->z*vx - acc->x*vz) + (mag->z*wx - mag->x*wz);  
	ez = (acc->x*vy - acc->y*vx) + (mag->x*wy - mag->y*wx);

	exInt = exInt + ex * ki;                                //�������л���
	eyInt = eyInt + ey * ki;
	ezInt = ezInt + ez * ki;

	// adjusted gyroscope measurements
	gyro->x = gyro->x + kp*ex + exInt;                                              //�����PI�󲹳��������ǣ����������Ư��
	gyro->y = gyro->y + kp*ey + eyInt;
	gyro->z = gyro->z + kp*ez + ezInt;                                          //�����gz����û�й۲��߽��н��������Ư�ƣ����ֳ����ľ��ǻ����������Լ�

	// integrate quaternion rate and normalise                           //��Ԫ�ص�΢�ַ���
	q0 = q0 + (-q1*gyro->x - q2*gyro->y - q3*gyro->z)*halfT;
	q1 = q1 + ( q0*gyro->x + q2*gyro->z - q3*gyro->y)*halfT;
	q2 = q2 + ( q0*gyro->y - q1*gyro->z + q3*gyro->x)*halfT;
	q3 = q3 + ( q0*gyro->z + q1*gyro->y - q2*gyro->x)*halfT;

	// normalise quaternion
	norm = sqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);

	q0 = q0 / norm;
	q1 = q1 / norm;
	q2 = q2 / norm;
	q3 = q3 / norm;

	att->pitch =  atan2(2.0f*(q0*q1 + q2*q3),q0*q0 - q1*q1 - q2*q2 + q3*q3)*rad_to_angle;
	att->roll =  asin(2.0f*(q0*q2 - q1*q3))*rad_to_angle;       
    att->yaw = atan2(2 * q1 * q2 + 2 * q0 * q3, -2 * q2*q2 - 2 * q3* q3 + 1)*rad_to_angle;
}


void imu_update(_angle* att,\
					_F32xyz* acc,\
					_F32xyz* gyro,\
					_F32xyz* mag) 
{
	float norm;
	float hx, hy, hz, bx, bz;
	float wx, wy, wz;
	float vx, vy, vz;
	float ex, ey, ez;
    if(acc->x * acc->y * acc->z==0)
        return;
    
	if(mag->x * mag->y * mag->z == 0)
		return;
	
	
    //��̬����ʱ����
    time_check(&IMU_TIM);
    
    //[ax,ay,az]�ǻ�������ϵ�¼��ٶȼƲ�õ���������(��ֱ����)
	norm = invSqrt(acc->x * acc->x + acc->y * acc->y + acc->z * acc->z);
	acc->x = acc->x * norm;
	acc->y = acc->y * norm;
	acc->z = acc->z * norm;
	
	
	norm = sqrt(mag->x * mag->x + mag->y * mag->y + mag->z * mag->z);       //mag���ݹ�һ��
	mag->x = mag->x / norm;
	mag->y = mag->y / norm;
	mag->z = mag->z / norm;
	
	hx = 2 * mag->x * (0.5f - q2*q2 - q3*q3) + 2 * mag->y * (q1*q2 - q0*q3) + 2 * mag->z * (q1*q3 + q0*q2);  
	hy = 2 * mag->x * (q1*q2 + q0*q3) + 2 * mag->y * (0.5f - q1*q1 - q3*q3) + 2 * mag->z * (q2*q3 - q0*q1);  
	hz = 2 * mag->x * (q1*q3 - q0*q2) + 2 * mag->y * (q2*q3 + q0*q1) + 2 * mag->z * (0.5f - q1*q1 -q2*q2);          
	bx = sqrt((hx*hx) + (hy*hy));  
	bz = hz;
	

	//VectorA = MatrixC * VectorB
	//VectorA ���ο���������ת���ڻ����µ�ֵ
	//MatrixC ����������ϵת��������ϵ����ת����  
	//VectorB ���ο�����������0,0,1��      
    //[vx,vy,vz]�ǵ�������ϵ����������[0,0,1]����DCM��ת����(C(n->b))����õ��Ļ�������ϵ�е���������(��ֱ����)    

    vx = 2.0f * (q1*q3 -q0*q2);//Mat.DCM_T[0][2];
    vy = 2.0f * (q2*q3 +q0*q1);//Mat.DCM_T[1][2];
    vz = 1.0f - 2.0f * q1*q1 - 2.0f * q2*q2;//Mat.DCM_T[2][2];
	
	
	wx = 2 * bx * (0.5f - q2*q2 - q3*q3) + 2 * bz * (q1*q3 - q0*q2);  
	wy = 2 * bx * (q1*q2 - q0*q3) + 2 * bz * (q0*q1 + q2*q3);  
	wz = 2 * bx * (q0*q2 + q1*q3) + 2 * bz * (0.5f - q1*q1 - q2*q2); 
    
    //��������ϵ��������˵õ�������������e���ǲ����õ���v����Ԥ��õ��� v^֮��������ת�������v������[ax,ay,az]��,v^����[vx,vy,vz]��
    //����������������DCM�������Ҿ���(����DCM�����е���Ԫ��)�������������þ��ǽ�bϵ��n��ȷ��ת��ֱ���غϡ�
    //ʵ����������������ֻ��bϵ��nϵ��XOYƽ���غ�����������z����ת��ƫ�������ٶȼ��޿��κΣ�
    //���ǣ����ڼ��ٶȼ��޷���֪z���ϵ���ת�˶������Ի���Ҫ�õشż�����һ��������
    //���������Ĳ���õ��Ľ��������������ģ������֮��н����ҵĳ˻�a��v=|a||v|sin��,
    //���ٶȼƲ����õ�������������Ԥ��õ��Ļ������������Ѿ�������λ����������ǵ�ģ��1��
    //Ҳ����˵���������Ĳ���������sin���йأ����ǶȺ�Сʱ�����������Խ����ڽǶȳ����ȡ�

    ex = acc->y * vz - acc->z * vy + (mag->y*wz - mag->z*wy);
	ey = acc->z * vx - acc->x * vz + (mag->z*wx - mag->x*wz);  
	ez = acc->x * vy - acc->y * vx + (mag->x*wy - mag->y*wx);
 
    //������������л���
	exInt = exInt + ex*ki;
	eyInt = eyInt + ey*ki;
	ezInt = ezInt + ez*ki;

    //ͨ������Kp��Ki�������������Կ��Ƽ��ٶȼ����������ǻ�����̬���ٶȡ�
	gyro->x = gyro->x + kp*ex + exInt;
	gyro->y = gyro->y + kp*ey + eyInt;
	gyro->z = gyro->z + kp*ez + ezInt;

    //һ�����������������Ԫ�� 
	q0 = q0 + (-q1 * gyro->x - q2 * gyro->y - q3 * gyro->z)* halfT;
	q1 = q1 + ( q0 * gyro->x + q2 * gyro->z - q3 * gyro->y)* halfT;
	q2 = q2 + ( q0 * gyro->y - q1 * gyro->z + q3 * gyro->x)* halfT;
	q3 = q3 + ( q0 * gyro->z + q1 * gyro->y - q2 * gyro->x)* halfT; 

    //��������������Ԫ�����й�һ���������õ������徭����ת����µ���Ԫ����
	norm = invSqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
	q0 = q0 * norm;
	q1 = q1 * norm;
	q2 = q2 * norm;
	q3 = q3 * norm;
    
	att->pitch =  atan2(2.0f*(q0*q1 + q2*q3),q0*q0 - q1*q1 - q2*q2 + q3*q3);
	att->roll =  asin(2.0f*(q0*q2 - q1*q3));       
 
    att->yaw = atan2(2 * q1 * q2 + 2 * q0 * q3, -2 * q2*q2 - 2 * q3* q3 + 1);
//	//z����ٶȻ��ֵ�ƫ����
//	att->yaw += _Mpu.deg_s.z  * dt;

	/*          ���ڵش���ν�����ǲ�  
	 * �ο�  http://baike.baidu.com/view/1239157.htm?fr=aladdin 
	 * ��ʽ
	 * XH = x*cos(P)+Y*sin(R)*sin(P)-Z*cos(R)*sin(p)
	 * YH = Y*cos(R)+Z*sin(R)
	 */
//	Xr = mag->x * COS(att->pitch/angle_to_rad)
//		+ mag->y * SIN(-att->pitch/angle_to_rad) * SIN(-att->roll/angle_to_rad)
//		- mag->z * COS(att->roll/angle_to_rad) * SIN(-att->pitch/angle_to_rad);
//	
//	Yr = mag->y * COS(att->roll/angle_to_rad) + mag->z * SIN(-att->roll/angle_to_rad);

//	att->yaw = atan2((double)Yr,(double)Xr) * rad_to_angle; // yaw 

	att->yaw *= rad_to_angle;
	att->roll *= rad_to_angle;
	att->pitch *= rad_to_angle;
	
}

void IMU_UPDATE(_angle* att,\
					_F32xyz* acc,\
					_F32xyz* gyro,\
					_F32xyz* mag)
{
	float norm;
	float vx, vy, vz;
	float ex, ey, ez;
	
 
    if(acc->x*acc->y*acc->z==0)
        return;
	
	if(mag->x * mag->y * mag->z == 0)
		return;
    
    //��̬����ʱ����
    time_check(&IMU_TIM);
    
    //[ax,ay,az]�ǻ�������ϵ�¼��ٶȼƲ�õ���������(��ֱ����)
	norm = invSqrt(acc->x*acc->x + acc->y*acc->y + acc->z*acc->z);
	acc->x = acc->x * norm;
	acc->y = acc->y * norm;
	acc->z = acc->z * norm;

	//VectorA = MatrixC * VectorB
	//VectorA ���ο���������ת���ڻ����µ�ֵ
	//MatrixC ����������ϵת��������ϵ����ת����  
	//VectorB ���ο�����������0,0,1��      
    //[vx,vy,vz]�ǵ�������ϵ����������[0,0,1]����DCM��ת����(C(n->b))����õ��Ļ�������ϵ�е���������(��ֱ����)    

    vx = 2.0f * (q1*q3 -q0*q2); //Mat.DCM_T[0][2];
    vy = 2.0f * (q2*q3 +q0*q1); //Mat.DCM_T[1][2];
    vz = 1.0f - 2.0f * q1*q1 - 2.0f * q2*q2;  //Mat.DCM_T[2][2];
    
    //��������ϵ��������˵õ�������������e���ǲ����õ���v����Ԥ��õ��� v^֮��������ת�������v������[ax,ay,az]��,v^����[vx,vy,vz]��
    //����������������DCM�������Ҿ���(����DCM�����е���Ԫ��)�������������þ��ǽ�bϵ��n��ȷ��ת��ֱ���غϡ�
    //ʵ����������������ֻ��bϵ��nϵ��XOYƽ���غ�����������z����ת��ƫ�������ٶȼ��޿��κΣ�
    //���ǣ����ڼ��ٶȼ��޷���֪z���ϵ���ת�˶������Ի���Ҫ�õشż�����һ��������
    //���������Ĳ���õ��Ľ��������������ģ������֮��н����ҵĳ˻�a��v=|a||v|sin��,
    //���ٶȼƲ����õ�������������Ԥ��õ��Ļ������������Ѿ�������λ����������ǵ�ģ��1��
    //Ҳ����˵���������Ĳ���������sin���йأ����ǶȺ�Сʱ�����������Խ����ڽǶȳ����ȡ�

	ex = acc->y*vz - acc->z*vy;
	ey = acc->z*vx - acc->x*vz;
	ez = acc->x*vy - acc->y*vx;
 
    //������������л���
	exInt = exInt + ex*ki*dt;
	eyInt = eyInt + ey*ki*dt;
	ezInt = ezInt + ez*ki*dt;

    //ͨ������Kp��Ki�������������Կ��Ƽ��ٶȼ����������ǻ�����̬���ٶȡ�
	gyro->x = gyro->x + kp*ex + exInt;
	gyro->y = gyro->y + kp*ey + eyInt;
	gyro->z = gyro->z + kp*ez + ezInt;

    //һ�����������������Ԫ�� 
	q0 = q0 + (-q1*gyro->x - q2*gyro->y - q3*gyro->z)* halfT;
	q1 = q1 + ( q0*gyro->x + q2*gyro->z - q3*gyro->y)* halfT;
	q2 = q2 + ( q0*gyro->y - q1*gyro->z + q3*gyro->x)* halfT;
	q3 = q3 + ( q0*gyro->z + q1*gyro->y - q2*gyro->x)* halfT; 

    //��������������Ԫ�����й�һ���������õ������徭����ת����µ���Ԫ����
	norm = invSqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
	q0 = q0 * norm;
	q1 = q1 * norm;
	q2 = q2 * norm;
	q3 = q3 * norm;
    
	att->pitch =  atan2(2.0f*(q0*q1 + q2*q3),q0*q0 - q1*q1 - q2*q2 + q3*q3) * rad_to_angle;
	att->roll =  asin(2.0f*(q0*q2 - q1*q3)) * rad_to_angle;       
 
    //z����ٶȻ��ֵ�ƫ����
    att->yaw += _Mpu.gyro_deg_s.z  * IMU_TIM.delta_time_s;
}





