#include "stdafx.h"
#include <cstdlib>
#include <iostream>
#include <fstream>

#include <ctime>

#include <Windows.h>
#include <NuiApi.h>
#include <opencv2\opencv.hpp>

//#include "player.h"
#define CORRECCION_X 0
#define CORRECCION_Y 35

#define PROFUNDIDAD_PULSACION 4000
#define JUGADOR_1 0
#define JUGADOR_2 1

#define TIME_LIMIT 50
#define FRAMES 3

//#define RESOLUCION NUI_IMAGE_RESOLUTION_1280x960
//#define RESX 1280
//#define RESY 960

#define RESOLUCION NUI_IMAGE_RESOLUTION_640x480
#define RESX 640
#define RESY 480

using namespace std;
using namespace cv;
extern int RANGO = 20;
extern int CERROR = 20;
extern int contador_frames = 0;

/*
typedef struct _punto {
        LONG x;
        LONG y;
        USHORT z;
}Punto3D;
 */

void dibuja_esqueleto(Mat m, NUI_SKELETON_DATA skeleton, bool flag);
void dibuja_hueso(Mat m, NUI_SKELETON_DATA skeleton, NUI_SKELETON_POSITION_INDEX jointFrom, NUI_SKELETON_POSITION_INDEX jointTo, bool flag);
void dibuja_hueso(Mat m, int frame);

void fusionar(const cv::Mat &fondo, const Mat &figura, Mat &salida, Point2i posicion);
bool comprueba_robo(NUI_SKELETON_DATA skeleton, Point p);

bool manos_arriba(Mat m, NUI_SKELETON_DATA skeleton);
bool lanza_bomba(Mat m, NUI_SKELETON_DATA skeleton, int id);

void Calibrar(Point2f &p);
void Calibrar(POINT &p);

void dibuja_ayuda_inicial(Mat m, Point p, int radio, bool pulsado);

void guardar_puntos_fichero();
void leer_puntos_fichero();
vector<Point2f> vector_puntos_fichero;
vector<int> vector_estados_fichero;
bool guardar_en_fichero=false;

int _tmain(int argc, _TCHAR* argv[]) {
    srand(time(0));
    setUseOptimized(true);

    INuiSensor* kinect;
    HRESULT hr = NuiCreateSensorByIndex(0, &kinect);
    if (FAILED(hr)) {
        cerr << "Error : NuiCreateSensorByIndex" << endl;
        return EXIT_FAILURE;
    }

    hr = kinect->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON);
    if (FAILED(hr)) {
        cerr << "Error : NuiInitialize" << endl;
        return -1;
    }

    HANDLE hSkeletonEvent = INVALID_HANDLE_VALUE;
    hSkeletonEvent = CreateEvent(nullptr, true, false, nullptr);
    hr = kinect->NuiSkeletonTrackingEnable(hSkeletonEvent, 0);
    if (FAILED(hr)) {
        cerr << "Error : NuiSkeletonTrackingEnable" << endl;
        return -1;
    }

    HANDLE hColorEvent = INVALID_HANDLE_VALUE;
    HANDLE hColorHandle = INVALID_HANDLE_VALUE;
    hColorEvent = CreateEvent(nullptr, true, false, nullptr);
    hr = kinect->NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, RESOLUCION, 0, 2, hColorEvent, &hColorHandle);
    if (FAILED(hr)) {
        cerr << "Error : NuiImageStreamOpen( COLOR )" << endl;
        return -1;
    }

	HANDLE hEvents[2] = {hColorEvent, hSkeletonEvent};


    namedWindow("Atracador");
    Mat moneda_am = imread("M_amarilla.png", CV_LOAD_IMAGE_UNCHANGED);
    Mat moneda_az = imread("M_azul.png", CV_LOAD_IMAGE_UNCHANGED);
    Mat moneda_gr = imread("M_gris.png", CV_LOAD_IMAGE_UNCHANGED);
    Mat fusion;
    vector<Mat> mizona;

    Point p1, p2, p3;
    int radio1 = moneda_am.rows;
    int radio2 = moneda_az.rows;
    int radio3 = moneda_gr.rows;

    p1.x = rand() % (RESX - radio1 * 2) + radio1;
    p1.y = rand() % (RESY - radio1 * 2) + radio1;

    p2.x = rand() % (RESX - radio2 * 2) + radio2;
    p2.y = rand() % (RESY - radio2 * 2) + radio2;

    p3.x = rand() % (RESX - radio3 * 2) + radio3;
    p3.y = rand() % (RESY - radio3 * 2) + radio3;

    bool jugar = false;
    string rango_str = "";

    bool bomba = false;
    unsigned time_msg_bomba = 50;

    bool robo = false;
    unsigned time_robo = 15;
    bool texto_robo = false;


    int monedas_capturadas[6] = {0};
    string num_monedas_str = "";

	bool muestra_ayuda = false;
    bool esqueleto = true;
	if (!guardar_en_fichero) 
		leer_puntos_fichero(); //Lee datos ayuda
	int f=0;

    while (true) {
        ResetEvent(hSkeletonEvent);
        ResetEvent(hColorEvent);

        //WaitForSingleObject(hSkeletonEvent, INFINITE);
        WaitForMultipleObjects(ARRAYSIZE(hEvents), hEvents, true, INFINITE);

        NUI_IMAGE_FRAME pColorImageFrame = {0};
        hr = kinect->NuiImageStreamGetNextFrame(hColorHandle, 0, &pColorImageFrame);
        if (FAILED(hr)) {
            cerr << "Error : NuiImageStreamGetNextFrame( COLOR )" << std::endl;
            return -1;
        }
		
        NUI_SKELETON_FRAME pSkeletonFrame = {0};
        hr = kinect->NuiSkeletonGetNextFrame(0, &pSkeletonFrame);
        if (FAILED(hr)) {
            cout << "Error : NuiSkeletonGetNextFrame" << endl;
            return -1;
        }

        INuiFrameTexture* pColorFrameTexture = pColorImageFrame.pFrameTexture;
        NUI_LOCKED_RECT sColorLockedRect;
        pColorFrameTexture->LockRect(0, &sColorLockedRect, nullptr, 0);

        Mat m(RESY, RESX, CV_8UC4, reinterpret_cast<uchar*> (sColorLockedRect.pBits));
        Mat zonas(200, RESX, CV_8UC4, Scalar(255, 255, 255));

        //Dibuja Esqueletos
        NUI_SKELETON_DATA skeleton;
        for (unsigned i = 0; i < NUI_SKELETON_COUNT; i++) {
            skeleton = pSkeletonFrame.SkeletonData[i];
            if (skeleton.eTrackingState == NUI_SKELETON_TRACKED) {//PLAYER DETECTADO
                dibuja_esqueleto(m, skeleton, esqueleto);
                //cout << "Jugador " << i << ": " << monedas_capturadas[i] << endl;
                if (!jugar) {
                    if (manos_arriba(m, skeleton))//INICIA JUEGO PARA PLAYER N
                        jugar = true;
                } else {
                    if (lanza_bomba(m, skeleton, JUGADOR_1))
                        bomba = true;
                    if (comprueba_robo(skeleton, p1)) {
                        robo = true;
                        monedas_capturadas[i]++;
                        num_monedas_str = static_cast<std::ostringstream*> (&(std::ostringstream() << monedas_capturadas[i]))->str();
                    }
                }
            }
        }

		Mat xxx;
		if (muestra_ayuda) {
			contador_frames++;
			if (contador_frames%FRAMES == 0 && contador_frames / FRAMES<vector_estados_fichero.size()) {
				f = contador_frames / FRAMES;
				Mat help(RESY, RESX, CV_8UC4, Scalar(255, 255, 255));
				dibuja_hueso(help, f * 76);
				resize(help, xxx, Size(200, 125), 0, 0, INTER_CUBIC);
				xxx.copyTo(zonas(Rect(0, 0, xxx.cols, xxx.rows)));
					//hconcat(zonas, xxx, zonas);
					//imshow("HELP", xxx);
			}
		}
        if (jugar) {
            if (bomba) {
                putText(zonas, "BOMBA PLAYER 1!!", Point(100, 100), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255, 0, 0), 2);
                time_msg_bomba--;
                if (!time_msg_bomba) {
                    time_msg_bomba = 50;
                    bomba = false;
                }

            }

            if (robo) {
                p1.x = rand() % (RESX - radio1 * 2) + radio1;
                p1.y = rand() % (RESY - radio1 * 2) + radio1;
                robo = false;
                texto_robo = true;
            }

            if (texto_robo) {
                putText(zonas, num_monedas_str, Point(50, 50), FONT_HERSHEY_COMPLEX_SMALL, 3, CV_RGB(20, 200, 20), 2);
                //putText(zonas, "BONUS!!", Point(100, 100), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255, 0, 0), 2);
                //time_msg_robo--;

                //if (!time_robo) {
                //	time_msg_robo = 50;
                //	texto_robo = false;
                //}

            }
			rango_str = "ERROR: ";
            rango_str += static_cast<std::ostringstream*> (&(std::ostringstream() << RANGO))->str();
            putText(zonas, rango_str, Point(100, 20), FONT_HERSHEY_COMPLEX_SMALL, 2, CV_RGB(255, 0, 0), 2);
            fusionar(m, moneda_am, fusion, p1);
            vconcat(fusion, zonas, fusion);
			m = fusion;
            //cv::imshow("Atracador", fusion);
        } else {

            putText(zonas, "MANOS ARRIBA!!!", Point(100, 100), FONT_HERSHEY_COMPLEX_SMALL, 2, CV_RGB(255, 0, 0), 2);
            //putText(m, "ESTO ES UN ATRACO!!!", Point(100, 250), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255, 0, 0), 2);			


            vconcat(m, zonas, m);
            //cv::imshow("Atracador", m);
        }
        cv::imshow("Atracador", m);

        //moneda_a.copyTo(ROI);
        //pintaCaja(m, p1);


        pColorFrameTexture->UnlockRect(0);
        kinect->NuiImageStreamReleaseFrame(hColorHandle, &pColorImageFrame);
		
		
        switch (waitKey(30)) {
            case VK_ESCAPE:
                if (guardar_en_fichero) {
                    guardar_puntos_fichero();
                }
                exit(0);
                break;
            case '+':
                RANGO++;
                break;
            case '-':
                RANGO--;
                break;
            case 'e':
                esqueleto = !esqueleto;
                break;
            case 'd':
                cout << endl;
                break;
			case 'w':
				guardar_en_fichero = true;
				break;
			case 'h':
				muestra_ayuda = !muestra_ayuda;
				break;
        }

		
    }


    kinect->NuiShutdown();
    kinect->NuiSkeletonTrackingDisable();
    CloseHandle(hColorEvent);
    CloseHandle(hSkeletonEvent);

    cv::destroyAllWindows();
    return 0;
}

void dibuja_hueso(Mat m, NUI_SKELETON_DATA skeleton, NUI_SKELETON_POSITION_INDEX jointFrom, NUI_SKELETON_POSITION_INDEX jointTo, bool flag) {

    NUI_SKELETON_POSITION_TRACKING_STATE jointFromState = skeleton.eSkeletonPositionTrackingState[jointFrom];
    NUI_SKELETON_POSITION_TRACKING_STATE jointToState = skeleton.eSkeletonPositionTrackingState[jointTo];

    if (jointFromState == NUI_SKELETON_POSITION_NOT_TRACKED || jointToState == NUI_SKELETON_POSITION_NOT_TRACKED) {
        return;
    }
    Point2f pointFrom, pointTo;
    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[jointFrom], &pointFrom.x, &pointFrom.y, RESOLUCION);
    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[jointTo], &pointTo.x, &pointTo.y, RESOLUCION);

    //Guardar puntos
    if (guardar_en_fichero) {
        vector_puntos_fichero.push_back(pointFrom);
        vector_puntos_fichero.push_back(pointTo);
        vector_estados_fichero.push_back(jointFromState);
        vector_estados_fichero.push_back(jointToState);
    }

    Calibrar(pointTo);
    Calibrar(pointFrom);

    if (flag) {
        if (jointFromState == NUI_SKELETON_POSITION_INFERRED || jointToState == NUI_SKELETON_POSITION_INFERRED) {
            line(m, pointFrom, pointTo, static_cast<Scalar> (Vec3b(0, 0, 255)), 2, CV_AA);
            circle(m, pointTo, 5, static_cast<Scalar> (Vec3b(0, 255, 255)), -1, CV_AA);
        }

        if (jointFromState == NUI_SKELETON_POSITION_TRACKED && jointToState == NUI_SKELETON_POSITION_TRACKED) {
            line(m, pointFrom, pointTo, static_cast<Scalar> (Vec3b(0, 255, 0)), 2, CV_AA);
            circle(m, pointTo, 5, static_cast<Scalar> (Vec3b(0, 255, 255)), -1, CV_AA);
        }
    }
}

void dibuja_hueso(Mat m, int frame) {
	int jointFromState;
	int jointToState;
	Point2f pointFrom, pointTo;
	cout << "TAMAÑO: " << vector_estados_fichero.size() << endl;
	for (unsigned i = 0; i < 76; i = i + 2) {
		jointFromState = vector_estados_fichero[i+frame];
		jointToState = vector_estados_fichero[(i+1)+frame];
		pointFrom = vector_puntos_fichero[i+frame];
		pointTo = vector_puntos_fichero[(i+1) + frame];

		Calibrar(pointTo);
		Calibrar(pointFrom);
		if (jointFromState == 1 || jointToState == 1) {
			line(m, pointFrom, pointTo, static_cast<Scalar> (Vec3b(0, 0, 255)), 2, CV_AA);
			circle(m, pointTo, 5, static_cast<Scalar> (Vec3b(0, 255, 255)), -1, CV_AA);
		}
		if (jointFromState == 2 && jointToState == 2) {
			line(m, pointFrom, pointTo, static_cast<Scalar> (Vec3b(0, 255, 0)), 2, CV_AA);
			circle(m, pointTo, 5, static_cast<Scalar> (Vec3b(0, 255, 255)), -1, CV_AA);
		}
	}
}

void dibuja_esqueleto(Mat m, NUI_SKELETON_DATA skeleton, bool flag) {

    //cabeza y espalda
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_HEAD, NUI_SKELETON_POSITION_SHOULDER_CENTER, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT, flag);

    //cadera
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SPINE, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_HIP_CENTER, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_LEFT, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_RIGHT, flag);

    //rodillas
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_HIP_LEFT, NUI_SKELETON_POSITION_KNEE_LEFT, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_HIP_RIGHT, NUI_SKELETON_POSITION_KNEE_RIGHT, flag);

    //rodillas
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_KNEE_LEFT, NUI_SKELETON_POSITION_ANKLE_LEFT, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_KNEE_RIGHT, NUI_SKELETON_POSITION_ANKLE_RIGHT, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_ANKLE_LEFT, NUI_SKELETON_POSITION_FOOT_LEFT, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_ANKLE_RIGHT, NUI_SKELETON_POSITION_FOOT_RIGHT, flag);

    //brazo izquierdo
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT, flag);

    //brazo derecho
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT, flag);
    dibuja_hueso(m, skeleton, NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT, flag);
}

void fusionar(const Mat &fondo, const Mat &figura, Mat &salida, Point2i posicion) {
    fondo.copyTo(salida);
    int fX, fY;
    double opacidad;
    unsigned char figuraPx, fondoPx;
    for (int y = max(posicion.y, 0); y < fondo.rows; y++) {
        fY = y - posicion.y;
        if (fY >= figura.rows)
            break;
        for (int x = max(posicion.x, 0); x < fondo.cols; x++) {
            fX = x - posicion.x;
            if (fX >= figura.cols)
                break;

            opacidad = ((double) figura.data[fY * figura.step + fX * figura.channels() + 3]) / 255.;
            for (int c = 0; opacidad > 0 && c < salida.channels(); c++) {
                figuraPx = figura.data[fY * figura.step + fX * figura.channels() + c];
                fondoPx = fondo.data[y * fondo.step + x * fondo.channels() + c];
                salida.data[y * salida.step + salida.channels() * x + c] = fondoPx * (1. - opacidad) + figuraPx * opacidad;
            }
        }
    }
}

bool comprueba_robo(NUI_SKELETON_DATA skeleton, const Point p) {
    static int time[NUI_SKELETON_COUNT] = {0};
    POINT p_mano_izq, p_mano_der, p_cabeza;

    USHORT pz_mano_izq = 0;
    USHORT pz_mano_der = 0;
    USHORT pz_cabeza = 0;

    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT], &p_mano_izq.x, &p_mano_izq.y, &pz_mano_izq, RESOLUCION);
    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT], &p_mano_der.x, &p_mano_der.y, &pz_mano_der, RESOLUCION);
    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HEAD], &p_cabeza.x, &p_cabeza.y, &pz_cabeza, RESOLUCION);

    Calibrar(p_mano_izq);
    Calibrar(p_mano_der);
    Calibrar(p_cabeza);

    bool robo = false;
    //cout << "CAB-IZQX: " << pz_cabeza - pz_mano_izq << " CAB-DERX: " << pz_cabeza - pz_mano_der << endl;
    if ((p_mano_izq.x >= (p.x - RANGO) && p_mano_izq.x <= (p.x + RANGO) && p_mano_izq.y >= (p.y - RANGO) && p_mano_izq.y <= (p.y + RANGO)) ||
            (p_mano_der.x >= (p.x - RANGO) && p_mano_der.x <= (p.x + RANGO) && p_mano_der.y >= (p.y - RANGO) && p_mano_der.y <= (p.y + RANGO)))
        robo = true;
    return robo;
}

bool manos_arriba(Mat m, NUI_SKELETON_DATA skeleton) {
    static int time[NUI_SKELETON_COUNT] = {0};
    Point pizq, pder;
    pizq.x = RESX / 2 - 50;
    pizq.y = 80;
    pder.x = RESX / 2 + 50;
    pder.y = 80;
    static bool manoder;
    static bool manoizq;

    POINT p_mano_izq, p_mano_der;
	POINT p_cabeza;
	//POINT p_codo_izq, p_codo_der;

	USHORT pz_mano_izq, pz_mano_der;
	USHORT pz_cabeza;
	//USHORT pz_codo_izq, pz_codo_der;

    bool estado = false;
    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT], &p_mano_izq.x, &p_mano_izq.y, &pz_mano_izq, RESOLUCION);
    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT], &p_mano_der.x, &p_mano_der.y, &pz_mano_der, RESOLUCION);
    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HEAD], &p_cabeza.x, &p_cabeza.y, &pz_cabeza, RESOLUCION);
    //NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_LEFT], &p_codo_izq.x, &p_codo_izq.y, &pz_codo_izq, RESOLUCION);
    //NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_RIGHT], &p_codo_der.x, &p_codo_der.y, &pz_codo_der, RESOLUCION);

    Calibrar(p_mano_izq);
    Calibrar(p_mano_der);
    Calibrar(p_cabeza);
    //Calibrar(p_codo_izq);
    //Calibrar(p_codo_der);

	pizq.x = p_cabeza.x - 50;
	pizq.y = p_cabeza.y - 80;
    if (p_mano_izq.x > pizq.x - RANGO && p_mano_izq.x < pizq.x + RANGO && p_mano_izq.y > pizq.y - RANGO && p_mano_izq.y < pizq.y + RANGO)
        manoizq = true;
    else
        manoizq = false;	
    dibuja_ayuda_inicial(m, Point(pizq.x, pizq.y), 300/(pz_cabeza/1000), manoizq);

	pder.x = p_cabeza.x + 50;
	pder.y = p_cabeza.y - 80;
    if (p_mano_der.x > pder.x - RANGO && p_mano_der.x < pder.x + RANGO && p_mano_der.y > pder.y - RANGO && p_mano_der.y < pder.y + RANGO)
        manoder = true;
    else
        manoder = false;
    dibuja_ayuda_inicial(m, Point(pder.x, pder.y), 300/(pz_cabeza/1000), manoder);

    if (manoizq && manoder) {
        //cout << "ENTRANDO EN ESTADO:              ESTADO IZQ: " << manoizq << "ESTADO DER: " << manoder << endl;
        estado = true;
    }

    return estado;
}

void dibuja_ayuda_inicial(Mat m, Point p, int radio, bool pulsado) {
    Scalar color;
    if (pulsado)
        color = static_cast<Scalar> (Vec3b(100, 200, 100));
    else
        color = static_cast<Scalar> (Vec3b(0, 255, 255));

    circle(m, p, radio, color, -1, CV_AA);

}

bool lanza_bomba(Mat m, NUI_SKELETON_DATA skeleton, int id) {
    static int time[NUI_SKELETON_COUNT] = {0};

    Point pizq, pder;

    POINT p_mano_izq, p_mano_der;
    POINT p_cabeza;
    POINT p_codo_izq, p_codo_der;
    USHORT pz_mano_izq, pz_mano_der;
    USHORT pz_cabeza;
    USHORT pz_codo_izq, pz_codo_der;

    bool retorno = false;
    static bool inicia = false;
	static bool gesto = false;

    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT], &p_mano_izq.x, &p_mano_izq.y, &pz_mano_izq, RESOLUCION);
    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT], &p_mano_der.x, &p_mano_der.y, &pz_mano_der, RESOLUCION);
    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_HEAD], &p_cabeza.x, &p_cabeza.y, &pz_cabeza, RESOLUCION);
    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_LEFT], &p_codo_izq.x, &p_codo_izq.y, &pz_codo_izq, RESOLUCION);
    NuiTransformSkeletonToDepthImage(skeleton.SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_RIGHT], &p_codo_der.x, &p_codo_der.y, &pz_codo_der, RESOLUCION);

	Calibrar(p_mano_izq);
    Calibrar(p_mano_der);
    Calibrar(p_cabeza);
    Calibrar(p_codo_izq);
    Calibrar(p_codo_der);

    pizq.x = p_cabeza.x - 125;
    pizq.y = p_cabeza.y;
    pder.x = p_cabeza.x + 125;
    pder.y = p_cabeza.y;

    //bool manoder = false;
    //bool manoizq = false;
	
    if (p_mano_izq.x >= pizq.x - RANGO && p_mano_izq.x <= pizq.x + RANGO && p_mano_der.x >= pder.x - RANGO && p_mano_der.x <= pder.x + RANGO
            && p_mano_izq.y >= pizq.y - RANGO && p_mano_izq.y <= pizq.y + RANGO && p_mano_der.y >= pder.y - RANGO && p_mano_der.y <= pder.y + RANGO) {
        cout << "Iniciando Deteccion de Gesto...: " << time[JUGADOR_1] << endl;
        inicia = true;
		gesto = true;
    }

    if (inicia) {
        time[JUGADOR_1]++;
        cout << time[JUGADOR_1] << endl;
        if ((p_mano_izq.y >= p_mano_der.y - CERROR && p_mano_izq.y <= p_mano_der.y + CERROR) && 
		   ((p_mano_izq.y>p_codo_izq.y) && (p_mano_der.y>p_codo_der.y)) && (p_mano_der.x - p_mano_izq.x < 150+CERROR) && gesto){
				cout << "POSEFINAL";
				time[JUGADOR_1] = 0;
				retorno = true;
				gesto = false;
        }
    }

    if (time[JUGADOR_1] >= TIME_LIMIT) {
        cout << "FUERA DE TIEMPO" << endl;
        inicia = false;
		gesto = false;
        time[JUGADOR_1] = 0;
    }
    //funcion_inicia_deteccion_gesto(m, p_mano_izq, pz_mano_izq, p_mano_der, pz_mano_der, p_cabeza, pz_cabeza, time[JUGADOR_1]);

    return retorno;
}

void Calibrar(Point2f &p) {
    p.x += CORRECCION_X;
    p.y += CORRECCION_Y;
}

void Calibrar(POINT &p) {
    p.x += CORRECCION_X;
    p.y += CORRECCION_Y;
}

void guardar_puntos_fichero() {
    int size = vector_estados_fichero.size();
    ofstream ofs("E:\\Dropbox\\Uni\\4\\NPI\\P1\\P1\\fichero.bin", ios::binary);
    ofs.write((char *) &size, sizeof (int));
    for (int i = 0; i < size; ++i) {
        ofs.write((char *) &vector_puntos_fichero[i], sizeof (Point2f));
    }
    for (int i = 0; i < size; ++i) {
        ofs.write((char *) &vector_estados_fichero[i], sizeof (int));
    }
    ofs.close();
}

void leer_puntos_fichero() {
    ifstream ifs("E:\\Dropbox\\Uni\\4\\NPI\\P1\\P1\\fichero.bin", ios::binary);
    int size;	
    ifs.read((char *) &size, sizeof (int));
	
    vector_puntos_fichero.resize(size);
    vector_estados_fichero.resize(size);

	//cout << "V: " << vector_estados_fichero.size() << endl;
    for (int i = 0; i < size; ++i) {
        ifs.read((char *) &vector_puntos_fichero[i], sizeof (Point2f));
    }
    for (int i = 0; i < size; ++i) {
        ifs.read((char *) &vector_estados_fichero[i], sizeof (int));
    }
    ifs.close();
}