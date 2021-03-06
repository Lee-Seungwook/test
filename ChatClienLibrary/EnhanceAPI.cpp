#include "pch.h"

#include "math.h"

#ifndef _DLL_ENHANCE_
#define _DLL_ENHANCE_
#endif // !_DLL_ENHANCE_

#include "EnhanceAPI.h"

void EnhanceAPI::APIINVERSE(IppByteImage& img)
{
	int size = img.GetSize();
	BYTE* p = img.GetPixels(); // 반전된 픽셀을 저장할 1차원 배열

	for (int i = 0; i < size; i++) // 영상 사이즈 만큼 반복
	{
		p[i] = 255 - p[i]; // 색상 반전 
	}
}

void EnhanceAPI::APIBrightness(IppByteImage& img, int n)
{
	int size = img.GetSize();
	BYTE* p = img.GetPixels();

	for (int i = 0; i < size; i++)
	{
		p[i] = limit(p[i] + n); // 0 또는 256보다 커지는것을 방지한다.
	}
}

void EnhanceAPI::APICBrightness(IppRgbImage& img, int n)
{
	int size = img.GetSize() * 3; // 트루 컬러 영상은 비트수가 3배 많아서 곱해주었다.
	RGBBYTE *p = img.GetPixels(); // 이미지를 1차적으로 저장한다. (트루 컬러 영상이기 때문이다.)
	BYTE *b = &p->b; // 각각의 픽셀 색상에 맞는 1차원 배열 선언
	BYTE *g = &p->g;
	BYTE *r = &p->r;

	for (int i = 0; i < size; i++)
	{
		b[i] = limit(b[i] + n);
		g[i] = limit(g[i] + n);
		r[i] = limit(r[i] + n);
	}
}

void EnhanceAPI::APIContrast(IppByteImage& img, int n)
{
	int size = img.GetSize();
	BYTE* p = img.GetPixels();

	for (int i = 0; i < size; i++)
	{
		p[i] = static_cast<BYTE>(limit(p[i] + (p[i] - 128) * n / 100));
	}
}

void EnhanceAPI::APICContrast(IppRgbImage& img, int n)
{
	int size = img.GetSize() * 3; // 트루 컬러 영상은 비트수가 3배 많아서 곱해주었다.
	RGBBYTE *p = img.GetPixels(); // 이미지를 1차적으로 저장한다. (트루 컬러 영상이기 때문이다.)
	BYTE *b = &p->b; // 각각의 픽셀 색상에 맞는 1차원 배열 선언
	BYTE *g = &p->g;
	BYTE *r = &p->r;

	for (int i = 0; i < size; i++)
	{
		b[i] = static_cast<BYTE>(limit(b[i] + (b[i] - 128) * n / 100));
		g[i] = static_cast<BYTE>(limit(g[i] + (g[i] - 128) * n / 100));
		r[i] = static_cast<BYTE>(limit(r[i] + (r[i] - 128) * n / 100));
	}
}

void EnhanceAPI::APIGammaCorrection(IppByteImage& img, float gamma)
{
	float inv_gamma = 1.f / gamma; // 결과가 실수로 나올수 있기 때문에 실수형 숫자를 사용, 스크롤바 감마지수로 나눈다.

	float gamma_table[256]; // 룩업 테이블 선언
	for (int i = 0; i < 256; i++)
		gamma_table[i] = pow((i / 255.f), inv_gamma); // 정의, pow연산은 시간 및 데이터 소모가 크다고 한다.

	int size = img.GetSize();
	BYTE*p = img.GetPixels();

	for (int i = 0; i < size; i++)
	{
		p[i] = static_cast<BYTE>(limit(gamma_table[p[i]] * 255 + 0.5f)); // 감마 보정, 마지막 실수를 더하는 것은 반올리 하기 위해서 이다.
	}
}

void EnhanceAPI::APICGammaCorrection(IppRgbImage& img, float gamma)
{
	float inv_gamma = 1.f / gamma; // 결과가 실수로 나올수 있기 때문에 실수형 숫자를 사용, 스크롤바 감마지수로 나눈다.

	float gamma_table[256]; // 룩업 테이블 선언
	for (int i = 0; i < 256; i++)
		gamma_table[i] = pow((i / 255.f), inv_gamma); // 정의, pow연산은 시간 및 데이터 소모가 크다고 한다.

	int size = img.GetSize() * 3;

	RGBBYTE*p = img.GetPixels();
	BYTE *b = &p->b; // 각각의 픽셀 색상에 맞는 1차원 배열 선언
	BYTE *g = &p->g;
	BYTE *r = &p->r;

	for (int i = 0; i < size; i++)
	{
		b[i] = static_cast<BYTE>(limit(gamma_table[b[i]] * 255 + 0.5f)); // 감마 보정, 마지막 실수를 더하는 것은 반올fla 하기 위해서 이다.
		g[i] = static_cast<BYTE>(limit(gamma_table[g[i]] * 255 + 0.5f));
		r[i] = static_cast<BYTE>(limit(gamma_table[r[i]] * 255 + 0.5f));
	}
}

void EnhanceAPI::APIHistogram(IppByteImage& img, float histo[256])
{
	int size = img.GetSize();
	BYTE* p = img.GetPixels();

	// 히스토그램 계산
	int cnt[256];
	memset(cnt, 0, sizeof(int) * 256);
	for (int i = 0; i < size; i++) // 픽셀의 개수를 계산
		cnt[p[i]]++;

	// 히스토그램 정규화(histogram normalization)
	for (int i = 0; i < 256; i++)
	{
		histo[i] = static_cast<float>(cnt[i]) / size; // cnt 배열에 저장된 값을 전체 픽셀의 개수로 나누어 정규화된 히스토그램 값을 histo 배열에 저장
	}
}

void EnhanceAPI::APIHistogramStrectching(IppByteImage& img)
{
	int size = img.GetSize();
	BYTE* p = img.GetPixels();

	// 최대, 최소 그레이스케일 값 계산
	BYTE gray_max, gray_min;
	gray_max = gray_min = p[0]; // 초기화
	for (int i = 1; i < size; i++)
	{
		if (gray_max < p[i]) gray_max = p[i];
		if (gray_min > p[i]) gray_min = p[i];
	}

	if (gray_max == gray_min)
		return;

	// 히스토그램 스트레칭
	for (int i = 0; i < size; i++)
	{
		p[i] = (p[i] - gray_min) * 255 / (gray_max - gray_min); // 공식 적용
	}
}

void EnhanceAPI::APIHistogramEqualization(IppByteImage& img)
{
	int size = img.GetSize();
	BYTE* p = img.GetPixels();

	// 히스토그램 계산
	float hist[256];
	APIHistogram(img, hist);

	// 히스토그램 누적 함수 계산
	float cdf[256] = { 0, 0, };
	cdf[0] = hist[0];
	for (int i = 1; i < 256; i++)
		cdf[i] = cdf[i - 1] + hist[i];

	// 히스토그램 균등화
	for (int i = 0; i < size; i++)
	{
		p[i] = static_cast<BYTE>(limit(cdf[p[i]] * 255));
	}
}

bool EnhanceAPI::APIAdd(IppByteImage& img1, IppByteImage& img2, IppByteImage& img3)
{
	int w = img1.GetWidth();
	int h = img1.GetHeight();

	if (w != img2.GetWidth() || h != img2.GetHeight()) // 입력받은 영상의 가로 세로 크기가 동일해야 연산을 수행
		return false;

	img3.CreateImage(w, h);

	int size = img3.GetSize();
	BYTE *p1 = img1.GetPixels();
	BYTE *p2 = img2.GetPixels();
	BYTE *p3 = img3.GetPixels();

	for (int i = 0; i < size; i++)
	{
		p3[i] = limit(p1[i] + p2[i]); // 덧셈 연산 수행
	}
}

bool EnhanceAPI::APISub(IppByteImage& img1, IppByteImage& img2, IppByteImage& img3)
{
	int w = img1.GetWidth();
	int h = img1.GetHeight();

	if (w != img2.GetWidth() || h != img2.GetHeight()) // 입력받은 영상의 가로 세로 크기가 동일해야 연산을 수행
		return false;

	img3.CreateImage(w, h);

	int size = img3.GetSize();
	BYTE *p1 = img1.GetPixels();
	BYTE *p2 = img2.GetPixels();
	BYTE *p3 = img3.GetPixels();

	for (int i = 0; i < size; i++)
	{
		p3[i] = limit(p1[i] - p2[i]); // 뺄셈 연산 수행
	}
}

bool EnhanceAPI::APIAve(IppByteImage& img1, IppByteImage& img2, IppByteImage& img3)
{
	int w = img1.GetWidth();
	int h = img1.GetHeight();

	if (w != img2.GetWidth() || h != img2.GetHeight()) // 입력받은 영상의 가로 세로 크기가 동일해야 연산을 수행
		return false;

	img3.CreateImage(w, h);

	int size = img3.GetSize();
	BYTE *p1 = img1.GetPixels();
	BYTE *p2 = img2.GetPixels();
	BYTE *p3 = img3.GetPixels();

	for (int i = 0; i < size; i++)
	{
		p3[i] = (p1[i] + p2[i]) / 2; // 평균 연산 수행 ( 평균 연산은 0 ~ 255 사이의 결과만 출력되기에 빠른 연산을 위해 limit함수를 제거)
	}
}

bool EnhanceAPI::APIDiff(IppByteImage& img1, IppByteImage& img2, IppByteImage& img3)
{
	int w = img1.GetWidth();
	int h = img1.GetHeight();

	if (w != img2.GetWidth() || h != img2.GetHeight()) // 입력받은 영상의 가로 세로 크기가 동일해야 연산을 수행
		return false;

	img3.CreateImage(w, h);

	int size = img3.GetSize();
	BYTE *p1 = img1.GetPixels();
	BYTE *p2 = img2.GetPixels();
	BYTE *p3 = img3.GetPixels();

	int diff;
	for (int i = 0; i < size; i++)
	{
		diff = p1[i] - p2[i]; // 영상의 뺄셈 연산 수행
		p3[i] = static_cast<BYTE>((diff >= 0) ? diff : -diff); // 절대값 변환
	}
}

bool EnhanceAPI::APIAND(IppByteImage& img1, IppByteImage& img2, IppByteImage& img3)
{
	int w = img1.GetWidth();
	int h = img1.GetHeight();

	if (w != img2.GetWidth() || h != img2.GetHeight()) // 입력받은 영상의 가로 세로 크기가 동일해야 연산을 수행
		return false;

	img3.CreateImage(w, h);

	int size = img3.GetSize();
	BYTE *p1 = img1.GetPixels();
	BYTE *p2 = img2.GetPixels();
	BYTE *p3 = img3.GetPixels();

	for (int i = 0; i < size; i++)
	{
		p3[i] = static_cast<BYTE>(p1[i] & p2[i]); // AND 연산 수행
	}
}

bool EnhanceAPI::APIOR(IppByteImage& img1, IppByteImage& img2, IppByteImage& img3)
{
	int w = img1.GetWidth();
	int h = img1.GetHeight();

	if (w != img2.GetWidth() || h != img2.GetHeight()) // 입력받은 영상의 가로 세로 크기가 동일해야 연산을 수행
		return false;

	img3.CreateImage(w, h);

	int size = img3.GetSize();
	BYTE *p1 = img1.GetPixels();
	BYTE *p2 = img2.GetPixels();
	BYTE *p3 = img3.GetPixels();

	for (int i = 0; i < size; i++)
	{
		p3[i] = static_cast<BYTE>(p1[i] | p2[i]); // OR 연산 수행
	}
}

void EnhanceAPI::APIBitPlane(IppByteImage& img1, IppByteImage& img2, int bit)
{
	img2.CreateImage(img1.GetWidth(), img1.GetHeight());

	int size = img1.GetSize();
	BYTE* p1 = img1.GetPixels();
	BYTE* p2 = img2.GetPixels();

	for (int i = 0; i < size; i++)
	{
		p2[i] = (p1[i] & (1 << bit)) ? 255 : 0; // 시프트 연산 수행하여 각 위치의 비트값과 비교
	}
}


