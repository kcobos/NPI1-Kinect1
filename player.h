#pragma once

class Player {

public:
	void dibuja_esqueleto(Mat m, NUI_SKELETON_DATA  skeleton);
	void dibuja_hueso(Mat m, NUI_SKELETON_DATA  skeleton, NUI_SKELETON_POSITION_INDEX jointFrom, NUI_SKELETON_POSITION_INDEX jointTo);

	bool comprueba_robo(NUI_SKELETON_DATA skeleton, Point p);

	bool manos_arriba(NUI_SKELETON_DATA skeleton);
	bool lanza_bomba(NUI_SKELETON_DATA skeleton, int id);

private:



};