#include "pch.h"

#ifndef _DLL_SEGMENT_
#define _DLL_SEGMENT_
#endif // !_DLL_SEGMENT_

#include "SegmentAPI.h"

// 영상 이진화 함수
// 입력 영상, 결과 영상, 임계값(임계값 보다 같거나 작으면 0, 크면 255로 픽셀 값 설정)
void SegmentAPI::APIBinarization(IppByteImage& imgSrc, IppByteImage& imgDst, int threshold)
{
	imgDst.CreateImage(imgSrc.GetWidth(), imgSrc.GetHeight());

	int size = imgSrc.GetSize();
	BYTE* pSrc = imgSrc.GetPixels();
	BYTE* pDst = imgDst.GetPixels();

	// 임계값을 이용한 픽셀값 설정
	for (int i = 0; i < size; i++)
	{
		pDst[i] = (pSrc[i] <= threshold) ? 0 : 255;
	}
}

// 임계값 결정 함수
int SegmentAPI::APIBinarizationIterative(IppByteImage& imgSrc)
{
	// 입력 영상의 히스토그램을 구하기 위해서 함수를 호출
	float hist[256] = { 0, };
	EnhanceAPI api;
	api.APIHistogram(imgSrc, hist); // 정규화된 히스토그램. hist 배열의 범위는 [0, 1]

	// 초기 임계값 결정 - 그레이스케일 값의 전체 평균

	int i, T, Told;

	float sum = 0.f;
	for (i = 0; i < 256; i++)
		sum += (i * hist[i]);

	T = static_cast<int>(sum + .5f);

	// 반복값에 의한 임계값 결정

	// 임계값 결정
	float a1, b1, u1, a2, b2, u2;
	do {
		Told = T;

		a1 = b1 = u1 = 0.f;
		for (i = 0; i <= Told; i++)
		{
			a1 += (i * hist[i]);
			b1 += hist[i];
		}

		if (b1 != 0.f)
			u1 = a1 / b1;

		a2 = b2 = u2 = 0.f;
		for (i = Told + 1; i < 256; i++)
		{
			a2 += (i * hist[i]);
			b2 += hist[i];
		}
		if (b2 != 0.f)
			u2 = a2 / b2;

		T = static_cast<int>((u1 + u2) / 2 + 0.5f);

	} while (T != Told); // 새로 갱신된 임계값과 이전의 임계값이 동일하면 반복을 중지한다.

	return T;
}

// 고전적 레이블링 기법을 수행하는 함수
// 입력 영상, 레이블링이 수행된 후 레이블 맵이 저장될 정수형 영상, 검출된 객체들의 레이블 정보를 담을 컨테이너
int SegmentAPI::APILabeling(IppByteImage& imgSrc, IppIntImage& imgDst, std::vector<APILabelInfo>& labels)
{
	int w = imgSrc.GetWidth();
	int h = imgSrc.GetHeight();

	BYTE** pSrc = imgSrc.GetPixels2D();

	//-------------------------------------------------------------------------
	// 임시로 레이블을 저장할 메모리 공간과 등가 테이블 생성
	//-------------------------------------------------------------------------

	IppIntImage imgMap(w, h);
	int** pMap = imgMap.GetPixels2D();

	const int MAX_LABEL = 100000;
	int eq_tbl[MAX_LABEL][2] = { { 0, }, };

	//-------------------------------------------------------------------------
	// 첫 번째 스캔 - 초기 레이블 지정 및 등가 테이블 생성
	//-------------------------------------------------------------------------

	register int i, j;
	int label = 0, maxl, minl, min_eq, max_eq;

	for (j = 1; j < h; j++)
		for (i = 1; i < w; i++)
		{
			if (pSrc[j][i] != 0)
			{
				// 바로 위 픽셀과 왼쪽 픽셀 모두에 레이블이 존재하는 경우
				if ((pMap[j - 1][i] != 0) && (pMap[j][i - 1] != 0))
				{
					if (pMap[j - 1][i] == pMap[j][i - 1])
					{
						// 두 레이블이 서로 같은 경우
						pMap[j][i] = pMap[j - 1][i];
					}
					else
					{
						// 두 레이블이 서로 다른 경우, 작은 레이블을 부여
						maxl = max(pMap[j - 1][i], pMap[j][i - 1]);
						minl = min(pMap[j - 1][i], pMap[j][i - 1]);

						pMap[j][i] = minl;

						// 등가 테이블 조정
						min_eq = min(eq_tbl[maxl][1], eq_tbl[minl][1]);
						max_eq = max(eq_tbl[maxl][1], eq_tbl[minl][1]);

						eq_tbl[eq_tbl[max_eq][1]][1] = min_eq;
					}
				}
				else if (pMap[j - 1][i] != 0)
				{
					// 바로 위 픽셀에만 레이블이 존재할 경우
					pMap[j][i] = pMap[j - 1][i];
				}
				else if (pMap[j][i - 1] != 0)
				{
					// 바로 왼쪽 픽셀에만 레이블이 존재할 경우
					pMap[j][i] = pMap[j][i - 1];
				}
				else
				{
					// 이웃에 레이블이 존재하지 않으면 새로운 레이블을 부여
					label++;
					pMap[j][i] = label;
					eq_tbl[label][0] = label;
					eq_tbl[label][1] = label;
				}
			}
		}

	//-------------------------------------------------------------------------
	// 등가 테이블 정리
	//-------------------------------------------------------------------------

	int temp;
	for (i = 1; i <= label; i++)
	{
		temp = eq_tbl[i][1];
		if (temp != eq_tbl[i][0])
			eq_tbl[i][1] = eq_tbl[temp][1];
	}

	// 등가 테이블의 레이블을 1부터 차례대로 증가시키기

	int* hash = new int[label + 1];
	memset(hash, 0, sizeof(int)*(label + 1));

	for (i = 1; i <= label; i++)
		hash[eq_tbl[i][1]] = eq_tbl[i][1];

	int label_cnt = 1;
	for (i = 1; i <= label; i++)
		if (hash[i] != 0)
			hash[i] = label_cnt++;

	for (i = 1; i <= label; i++)
		eq_tbl[i][1] = hash[eq_tbl[i][1]];

	delete[] hash;

	//-------------------------------------------------------------------------
	// 두 번째 스캔 - 등가 테이블을 이용하여 모든 픽셀에 고유의 레이블 부여
	//-------------------------------------------------------------------------

	imgDst.CreateImage(w, h);
	int** pDst = imgDst.GetPixels2D();

	int idx;
	for (j = 1; j < h; j++)
		for (i = 1; i < w; i++)
		{
			if (pMap[j][i] != 0)
			{
				idx = pMap[j][i];
				pDst[j][i] = eq_tbl[idx][1];
			}
		}

	//-------------------------------------------------------------------------
	// IppLabelInfo 정보 작성
	//-------------------------------------------------------------------------

	labels.resize(label_cnt - 1);

	APILabelInfo* pLabel;
	for (j = 1; j < h; j++)
		for (i = 1; i < w; i++)
		{
			if (pDst[j][i] != 0)
			{
				pLabel = &labels.at(pDst[j][i] - 1);
				pLabel->pixels.push_back(APIPoint(i, j));
				pLabel->cx += i;
				pLabel->cy += j;

				if (i < pLabel->minx) pLabel->minx = i;
				if (i > pLabel->maxx) pLabel->maxx = i;
				if (j < pLabel->miny) pLabel->miny = j;
				if (j > pLabel->maxy) pLabel->maxy = j;
			}
		}

	for (APILabelInfo& label : labels)
	{
		label.cx /= label.pixels.size();
		label.cy /= label.pixels.size();
	}

	return (label_cnt - 1);
}

//외곽선 검출
// 입력 영상, sx, sy는 외곽선 추적을 시작할 객체의 시작 좌표, 외곽선 픽셀 좌표를 저장할 컨테이너
void SegmentAPI::APIContourTracing(IppByteImage& imgSrc, int sx, int sy, std::vector<APIPoint>& cp)
{
	int w = imgSrc.GetWidth();
	int h = imgSrc.GetHeight();

	BYTE** pSrc = imgSrc.GetPixels2D();

	// 외곽선 좌표를 저장할 구조체 초기화
	cp.clear();

	// 외곽선 추적 시작 픽셀이 객체가 아니면 종료
	if (pSrc[sy][sx] != 255)
		return;

	int x, y, nx, ny;
	int d, cnt;
	int  dir[8][2] = { // 진행 방향을 나타내는 배열
		{  1,  0 },
		{  1,  1 },
		{  0,  1 },
		{ -1,  1 },
		{ -1,  0 },
		{ -1, -1 },
		{  0, -1 },
		{  1, -1 }
	};

	x = sx;
	y = sy;
	d = cnt = 0;

	while (1)
	{
		nx = x + dir[d][0];
		ny = y + dir[d][1];

		if (nx < 0 || nx >= w || ny < 0 || ny >= h || pSrc[ny][nx] == 0)
		{
			// 진행 방향에 있는 픽셀이 객체가 아닌 경우,
			// 시계 방향으로 진행 방향을 바꾸고 다시 시도한다.

			if (++d > 7) d = 0;
			cnt++;

			// 8방향 모두 배경인 경우 
			if (cnt >= 8)
			{
				cp.push_back(APIPoint(x, y));
				break;  // 외곽선 추적을 끝냄.
			}
		}
		else
		{
			// 진행 방향의 픽셀이 객체일 경우, 현재 점을 외곽선 정보에 저장
			cp.push_back(APIPoint(x, y));

			// 진행 방향으로 이동
			x = nx;
			y = ny;

			// 방향 정보 초기화
			cnt = 0;
			d = (d + 6) % 8;	// d = d - 2 와 같은 형태
		}

		// 시작점으로 돌아왔고, 진행 방향이 초기화된 경우
		// 외곽선 추적을 끝낸다.
		if (x == sx && y == sy && d == 0)
			break;
	}
}

// 모폴로지 침식 연산
void SegmentAPI::APIMorphologyErosion(IppByteImage& imgSrc, IppByteImage& imgDst)
{
	int i, j;
	int w = imgSrc.GetWidth();
	int h = imgSrc.GetHeight();

	imgDst = imgSrc;

	BYTE** pDst = imgDst.GetPixels2D();
	BYTE** pSrc = imgSrc.GetPixels2D();

	for (j = 1; j < h - 1; j++)
		for (i = 1; i < w - 1; i++)
		{
			if (pSrc[j][i] != 0)
			{
				if (pSrc[j - 1][i] == 0 || pSrc[j - 1][i + 1] == 0 ||
					pSrc[j][i - 1] == 0 || pSrc[j][i + 1] == 0 ||
					pSrc[j + 1][i - 1] == 0 || pSrc[j + 1][i] == 0 ||
					pSrc[j + 1][i + 1] == 0 || pSrc[j - 1][i - 1] == 0)
				{
					pDst[j][i] = 0;
				}
			}
		}
}

// 모폴로지 팽창 연산
void SegmentAPI::APIMorphologyDilation(IppByteImage& imgSrc, IppByteImage& imgDst)
{
	int i, j;
	int w = imgSrc.GetWidth();
	int h = imgSrc.GetHeight();

	imgDst = imgSrc;

	BYTE** pDst = imgDst.GetPixels2D();
	BYTE** pSrc = imgSrc.GetPixels2D();

	for (j = 1; j < h - 1; j++)
		for (i = 1; i < w - 1; i++)
		{
			if (pSrc[j][i] == 0)
			{
				if (pSrc[j - 1][i] != 0 || pSrc[j - 1][i + 1] != 0 ||
					pSrc[j][i - 1] != 0 || pSrc[j][i + 1] != 0 ||
					pSrc[j + 1][i - 1] != 0 || pSrc[j + 1][i] != 0 ||
					pSrc[j + 1][i + 1] != 0 || pSrc[j - 1][i - 1] != 0)
				{
					pDst[j][i] = 255;
				}
			}
		}
}

// 모폴로지 열기 연산
void SegmentAPI::APIMorphologyOpening(IppByteImage& imgSrc, IppByteImage& imgDst)
{
	IppByteImage imgTmp;
	APIMorphologyErosion(imgSrc, imgTmp); // 침식
	APIMorphologyDilation(imgTmp, imgDst); // 팽창
}

// 모폴로지 닫기 연산
void SegmentAPI::APIMorphologyClosing(IppByteImage& imgSrc, IppByteImage& imgDst)
{
	IppByteImage imgTmp;
	APIMorphologyDilation(imgSrc, imgTmp); // 팽창
	APIMorphologyErosion(imgTmp, imgDst); // 침식
}

// 그레이 스케일 모폴로지 침식
void SegmentAPI::APIMorphologyGrayErosion(IppByteImage& imgSrc, IppByteImage& imgDst)
{
	int i, j, m, n, x, y, pmin;
	int w = imgSrc.GetWidth();
	int h = imgSrc.GetHeight();

	imgDst = imgSrc;

	BYTE** pDst = imgDst.GetPixels2D();
	BYTE** pSrc = imgSrc.GetPixels2D();

	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++)
		{
			pmin = 255;

			for (n = -1; n <= 1; n++)
				for (m = -1; m <= 1; m++)
				{
					x = i + m;
					y = j + n;

					if (x >= 0 && x < w && y >= 0 && y < h)
					{
						if (pSrc[y][x] < pmin)
							pmin = pSrc[y][x];
					}
				}

			pDst[j][i] = pmin;
		}
}

// 그레이 스케일 모폴로지 팽창
void SegmentAPI::APIMorphologyGrayDilation(IppByteImage& imgSrc, IppByteImage& imgDst)
{
	int i, j, m, n, x, y, pmax;
	int w = imgSrc.GetWidth();
	int h = imgSrc.GetHeight();

	imgDst = imgSrc;

	BYTE** pDst = imgDst.GetPixels2D();
	BYTE** pSrc = imgSrc.GetPixels2D();

	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++)
		{
			pmax = 0;

			for (n = -1; n <= 1; n++)
				for (m = -1; m <= 1; m++)
				{
					x = i + m;
					y = j + n;

					if (x >= 0 && x < w && y >= 0 && y < h)
					{
						if (pSrc[y][x] > pmax)
							pmax = pSrc[y][x];
					}
				}

			pDst[j][i] = pmax;
		}
}

// 그레이스케일 영상 열기
void SegmentAPI::APIMorphologyGrayOpening(IppByteImage& imgSrc, IppByteImage& imgDst)
{
	IppByteImage imgTmp;
	APIMorphologyGrayErosion(imgSrc, imgTmp); // 침식
	APIMorphologyGrayDilation(imgTmp, imgDst); // 팽창
}

// 그레이스케일 영상 닫기
void SegmentAPI::APIMorphologyGrayClosing(IppByteImage& imgSrc, IppByteImage& imgDst)
{
	IppByteImage imgTmp;
	APIMorphologyGrayDilation(imgSrc, imgTmp); // 팽창
	APIMorphologyGrayErosion(imgTmp, imgDst); // 침식
}

int SegmentAPI::APILabelingDot(IppByteImage& imgSrc, IppIntImage& imgDst, std::vector<APILabelInfo>& labels)
{
	int w = imgSrc.GetWidth();
	int h = imgSrc.GetHeight();

	BYTE** pSrc = imgSrc.GetPixels2D();

	//-------------------------------------------------------------------------
	// 임시로 레이블을 저장할 메모리 공간과 등가 테이블 생성
	//-------------------------------------------------------------------------

	IppIntImage imgMap(w, h);
	int** pMap = imgMap.GetPixels2D();

	const int MAX_LABEL = 100000;
	int eq_tbl[MAX_LABEL][2] = { { 0, }, };

	//-------------------------------------------------------------------------
	// 첫 번째 스캔 - 초기 레이블 지정 및 등가 테이블 생성
	//-------------------------------------------------------------------------

	register int i, j;
	int label = 0, maxl, minl, min_eq, max_eq;

	for (j = 1; j < h; j++)
		for (i = 1; i < w; i++)
		{
			if (pSrc[j][i] != 0)
			{
				// 바로 위 픽셀과 왼쪽 픽셀 모두에 레이블이 존재하는 경우
				if ((pMap[j - 1][i] != 0) && (pMap[j][i - 1] != 0))
				{
					if (pMap[j - 1][i] == pMap[j][i - 1])
					{
						// 두 레이블이 서로 같은 경우
						pMap[j][i] = pMap[j - 1][i];
					}
					else
					{
						// 두 레이블이 서로 다른 경우, 작은 레이블을 부여
						maxl = max(pMap[j - 1][i], pMap[j][i - 1]);
						minl = min(pMap[j - 1][i], pMap[j][i - 1]);

						pMap[j][i] = minl;

						// 등가 테이블 조정
						min_eq = min(eq_tbl[maxl][1], eq_tbl[minl][1]);
						max_eq = max(eq_tbl[maxl][1], eq_tbl[minl][1]);

						eq_tbl[eq_tbl[max_eq][1]][1] = min_eq;
					}
				}
				else if (pMap[j - 1][i] != 0)
				{
					// 바로 위 픽셀에만 레이블이 존재할 경우
					pMap[j][i] = pMap[j - 1][i];
				}
				else if (pMap[j][i - 1] != 0)
				{
					// 바로 왼쪽 픽셀에만 레이블이 존재할 경우
					pMap[j][i] = pMap[j][i - 1];
				}
				else
				{
					// 이웃에 레이블이 존재하지 않으면 새로운 레이블을 부여
					label++;
					pMap[j][i] = label;
					eq_tbl[label][0] = label;
					eq_tbl[label][1] = label;
				}
			}
		}

	//-------------------------------------------------------------------------
	// 등가 테이블 정리
	//-------------------------------------------------------------------------

	int temp;
	for (i = 1; i <= label; i++)
	{
		temp = eq_tbl[i][1];
		if (temp != eq_tbl[i][0])
			eq_tbl[i][1] = eq_tbl[temp][1];
	}

	// 등가 테이블의 레이블을 1부터 차례대로 증가시키기

	int* hash = new int[label + 1];
	memset(hash, 0, sizeof(int)*(label + 1));

	for (i = 1; i <= label; i++)
		hash[eq_tbl[i][1]] = eq_tbl[i][1];

	int label_cnt = 1;
	for (i = 1; i <= label; i++)
		if (hash[i] != 0)
			hash[i] = label_cnt++;

	for (i = 1; i <= label; i++)
		eq_tbl[i][1] = hash[eq_tbl[i][1]];

	delete[] hash;

	//-------------------------------------------------------------------------
	// 두 번째 스캔 - 등가 테이블을 이용하여 모든 픽셀에 고유의 레이블 부여
	//-------------------------------------------------------------------------

	imgDst.CreateImage(w, h);
	int** pDst = imgDst.GetPixels2D();

	int idx;
	for (j = 1; j < h; j++)
		for (i = 1; i < w; i++)
		{
			if (pMap[j][i] != 0)
			{
				idx = pMap[j][i];
				pDst[j][i] = eq_tbl[idx][1];
			}
		}

	//-------------------------------------------------------------------------
	// IppLabelInfo 정보 작성
	//-------------------------------------------------------------------------

	labels.resize(label_cnt - 1);

	APILabelInfo* pLabel;
	for (j = 1; j < h; j++)
		for (i = 1; i < w; i++)
		{
			if (pDst[j][i] != 0)
			{
				pLabel = &labels.at(pDst[j][i] - 1);
				pLabel->pixels.push_back(APIPoint(i, j));
				pLabel->cx += i;
				pLabel->cy += j;

				if (i < pLabel->minx) pLabel->minx = i;
				if (i > pLabel->maxx) pLabel->maxx = i;
				if (j < pLabel->miny) pLabel->miny = j;
				if (j > pLabel->maxy) pLabel->maxy = j;
			}
		}

	for (APILabelInfo& label : labels)
	{
		label.cx /= label.pixels.size();
		label.cy /= label.pixels.size();
	}

	return (label_cnt - 1);
}