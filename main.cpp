#define _CRT_SECURE_NO_WARNINGS
#include "header.h"

int main()
{
	FILE* image = fopen("D:\\image.bmp", "rb");
	BITMAPFILEHEADER FILEHEADER;
	BITMAPINFOHEADER INFOHEADER;
	fread(&FILEHEADER, sizeof(BITMAPFILEHEADER), 1, image);
	fread(&INFOHEADER, sizeof(BITMAPINFOHEADER), 1, image);

	int ch = 3;		// Число каналов изображения
	int padding = 0;	// Отступы (если ширина изображения в пикселях не кратна 4, то чтобы записать в буфер "чистый" растр 
						// при чтении в каждой строке необходимо пропускать число байт, не достающих для того, чтобы ширина в байтах была кратна 4).
	if (((INFOHEADER.biWidth * 3) % 4) != 0) padding = 4 - (INFOHEADER.biWidth * 3) % 4;
	BYTE* buffer = new BYTE[(INFOHEADER.biWidth * ch) * INFOHEADER.biHeight];		// Буфер, содержащий растр исходного изображения
	for (int i = 0; i < INFOHEADER.biHeight; i++)
	{
		fread(&buffer[i * INFOHEADER.biWidth * ch], sizeof(BYTE), INFOHEADER.biWidth * ch, image);
		if (padding != 0) fseek(image, padding, SEEK_CUR);
	}

	BYTE* buffer_ch = new BYTE[INFOHEADER.biWidth * INFOHEADER.biHeight];		// Буфер, содержащий информацию первого канала (синего) исходного изображенмя
	for (int i = 0, j = 0; i < INFOHEADER.biWidth * INFOHEADER.biHeight; i++, j += 3)
	{
		buffer_ch[i] = buffer[j];
	}

	double angle = 129;		// Угол поворота в градусах
	double a = angle / 180. * PI;	 // Угол поворота в радианах
	int width = INFOHEADER.biWidth;
	int height = INFOHEADER.biHeight;
	long newWidth, newHeight;

	Points center{ 0, 0 };	// Точка, вокруг которой происходит вращение
	std::vector<Points> Point{ { 0, 0 }, { 0, height }, { width, height }, { width, 0 } };		// Вершины исходного изображения
	std::vector<Points> VPoint(4);		// Вершины в новом буфере
	Point[0].x = 0;
	Point[0].y = 0;
	Point[1].x = 0;
	Point[1].y = height;
	Point[2].x = width;
	Point[2].y = height;
	Point[3].x = width;
	Point[3].y = 0;

	double fX = 0; // fX и fY - переменные для записи результатов обратного геометрического преобразования
	double fY = 0;
	long nMaxX, nMaxY, nMinX, nMinY;
	nMaxX = nMaxY = LONG_MIN;
	nMinX = nMinY = LONG_MAX;
	for (int i = 0; i < 4; i++)
	{
		CoordTransform(Point[i].x, Point[i].y, fX, fY, a);
		VPoint[i].x = (long)(fX + 0.5);
		VPoint[i].y = (long)(fY + 0.5);
		if (VPoint[i].x > nMaxX) nMaxX = VPoint[i].x;
		if (VPoint[i].y > nMaxY) nMaxY = VPoint[i].y;
		if (VPoint[i].x < nMinX) nMinX = VPoint[i].x;
		if (VPoint[i].y < nMinY) nMinY = VPoint[i].y;
	}

	newWidth = nMaxX - nMinX;
	newHeight = nMaxY - nMinY;
	std::vector<int> arrX;
	std::vector<int> arrY;
	for (int i = 0; i < VPoint.size(); i++)
	{
		arrX.push_back(VPoint[i].x);
		arrY.push_back(VPoint[i].y);
	}

	int OffsetX = (*std::min_element(arrX.begin(), arrX.end()));	// Сдвиги, связанные с переходом одной или нескольких вершин
	int OffsetY = (*std::min_element(arrY.begin(), arrY.end()));	// в отрицательную область координатной сетки при повороте изображения, которые необходимо добавить по осям X и Y

	BYTE* buffer_rotation = new BYTE[newWidth * newHeight];		// Буфер для записи растра повернутого изображения

	auto start = std::chrono::system_clock::now();
	for (int i = 0; i < newHeight; i++)
	{
		for (int j = 0; j < newWidth; j++)
		{
			CoordTransform(j + OffsetX, i + OffsetY, fX, fY, -a);
			if ((fX < 0) || (fY < 0) || ((int)(fX + 0.5) >= width) || ((int)(fY + 0.5) >= height))
			{
				buffer_rotation[i * newWidth + j] = 0;
			}
			else
			{
				//buffer_rotation[i * newWidth + j] = buffer_ch[(int)(fY + 0.5) * width + (int)(fX + 0.5)];		// Интерполяция по ближайшему соседу
				BilenearInterp(fX, fY, width, height, newWidth, newHeight, buffer_ch, buffer_rotation, i, j);	// Билинейная интерполяция
			}
		}
	}
	auto end = std::chrono::system_clock::now();
	std::chrono::duration<long double> elapsed_seconds = end - start;
	std::time_t end_time = std::chrono::system_clock::to_time_t(end);
	std::cout << "Time: " << std::fixed << elapsed_seconds.count() << std::endl;
	FILE* rotated_image = fopen("D:\\BilenearImg.bmp", "wb");
	BITMAPFILEHEADER NEWFILEHEADER;
	BITMAPINFOHEADER NEWINFOHEADER;
	NEWFILEHEADER = FILEHEADER;
	NEWINFOHEADER = INFOHEADER;
	NEWINFOHEADER.biWidth = newWidth;
	NEWINFOHEADER.biHeight = newHeight;
	fwrite(&NEWFILEHEADER, sizeof(BITMAPFILEHEADER), 1, rotated_image);
	fwrite(&NEWINFOHEADER, sizeof(BITMAPINFOHEADER), 1, rotated_image);
	BYTE* buffer_rot = new BYTE[newWidth * ch * newHeight];		// Трехканальный буфер, содержащий растр повернутого изображения
	for (int i = 0; i < NEWINFOHEADER.biWidth * NEWINFOHEADER.biHeight; i++)
	{
		buffer_rot[3 * i] = buffer_rot[3 * i + 1] = buffer_rot[3 * i + 2] = buffer_rotation[i];
	}

	int rotate_padding = 0;
	if (((newWidth * 3) % 4) != 0) rotate_padding = 4 - (newWidth * 3) % 4;			// Отступы для повернутого изображения (если ширина нового изображения
																					// в байтах не кратна 4, то при записи необходимо вручную дополнить каждую строку
																					// "нулями", чтобы ширина в байтах стала кратна 4
	BYTE pad[3] = { 0, 0, 0 };
	for (int i = 0; i < newHeight; i++)
	{
		fwrite(&buffer_rot[i * newWidth * ch], sizeof(BYTE), newWidth * ch, rotated_image);
		fwrite(pad, sizeof(BYTE), rotate_padding, rotated_image);
	}
	return 0;
}


void CoordTransform(long nPix, long nLine, double& fX, double& fY, double a)	// Функция обратного геометричекого преобразования
{
	fX = nPix * cos(a) - nLine * sin(a);
	fY = nPix * sin(a) + nLine * cos(a);
}

// Функция билинейной интерполяции
void BilenearInterp(double fX, double fY, int width, int height, long newWidth, long newHeight, BYTE* buffer_ch, BYTE* buffer_rotation, int i, int j)
{
	double dX, dY;
	dX = fX - trunc(fX);
	dY = fY - trunc(fY);
	double wht_sum = 0;
	wht_sum += buffer_ch[(int)(fY + 0.5) * width + (int)(fX + 0.5)] * (1 - dX) * (1 - dY);

	if ((int)(fX + 1) >= width)
	{
		wht_sum += 0;
	}
	else
	{
		wht_sum += buffer_ch[(int)fY * width + (int)(fX + 1)] * dX * (1 - dY);
	}
	if ((int)(fY + 1) >= height)
	{
		wht_sum += 0;
	}
	else
	{
		wht_sum += buffer_ch[(int)(fY + 1) * width + (int)fX] * (1 - dX) * dY;
	}

	if ((int)(fX + 1) >= width || (int)(fY + 1) >= height)
	{
		wht_sum += 0;
	}
	else
	{
		wht_sum += buffer_ch[(int)(fY + 1) * width + (int)(fX + 1)] * dX * dY;
	}

	buffer_rotation[i * newWidth + j] = wht_sum;
}

