#pragma once

typedef struct tagLDDATA
{
	int nLD;
	int PreRow;
	int PreCol;
} LDDATA, *LPLDDATA;

//*lpPos==0 diag, *lpPos==1 left, *lpPos==2 above
int MinLD (int diag, int left, int above, int *lpPos)
{
	int min;

	min = diag;
	*lpPos=0;
	if (left < min)
	{
		min = left;
		*lpPos=1;
	}
	if (above < min)
	{
		min = above;
		*lpPos=2;
	}
	return min;
}

bool TraceBack(LPLDDATA lpLdData, int x, int y, int col, BYTE *lpIndexS, BYTE *lpIndexT)
{
	int px, py;

	if (0==x || 0==y)
		return false;
	px=lpLdData[x+y*col].PreCol;
	py=lpLdData[x+y*col].PreRow;
	if(lpLdData[x+y*col].nLD==lpLdData[px+py*col].nLD)//相同
	{
		lpIndexS[x-1]=100;
		lpIndexT[y-1]=100;
	}
	else if (1==x-px && 1==y-py)//替换
	{
		lpIndexS[x-1]=10;
		lpIndexT[y-1]=10;
	}
	if(-1==lpLdData[px+py*col].PreCol || -1==lpLdData[px+py*col].PreRow)
		return true;
	TraceBack(lpLdData, px, py, col, lpIndexS, lpIndexT);
	return false;
}

//BYTE *lpIndexS, BYTE *lpIndexT为存放回溯数据的数组，用来记录两个字符串中相同字符的索引
int LD (LPCTSTR s, LPCTSTR t, BYTE *lpIndexS, BYTE *lpIndexT)
{
	CString cstrS=s, cstrT=t;
	int row, col, i, j, cost, nPos, result;
	TCHAR tchS, tchT;
	LPLDDATA lpLdData;

	// Step 1
	col=cstrS.GetLength()+1;
	row=cstrT.GetLength()+1;
	if (1==col)
		return row-1;
	if (1==row)
		return col-1;

	lpLdData=(LPLDDATA)malloc(row*col*sizeof(LDDATA));
	memset(lpLdData, -1, row*col*sizeof(LDDATA));
	if (NULL==lpLdData)
		return -1;

	// Step 2
	for (i=0; i<row; i++)//初始化首列
		lpLdData[i*col].nLD=i;
	for (i=0; i<col; i++)//初始化首行
		lpLdData[i].nLD=i;

	// Step 3
	for (i=1; i<col; i++)
	{
		tchS=cstrS.GetAt(i-1);

		// Step 4
		for (j=1; j<row; j++)
		{
			tchT=cstrT.GetAt(j-1);

			// Step 5
			if (tchS==tchT)
				cost=0;
			else
				cost=1;

			// Step 6
			lpLdData[i+j*col].nLD=MinLD(lpLdData[(i-1)+(j-1)*col].nLD+cost,
								lpLdData[(i-1)+j*col].nLD+1,
								lpLdData[i+(j-1)*col].nLD+1,
								&nPos);
			//记录下从哪个单元格得出的LD值，便于回溯
			switch (nPos)
			{
			case 0://diag
				lpLdData[i+j*col].PreCol=i-1;
				lpLdData[i+j*col].PreRow=j-1;
				break;
			case 1://left
				lpLdData[i+j*col].PreCol=i-1;
				lpLdData[i+j*col].PreRow=j;
				break;
			case 2://above
				lpLdData[i+j*col].PreCol=i;
				lpLdData[i+j*col].PreRow=j-1;
			}
		}
	}

	// Step 7
	result=lpLdData[(col-1)+(row-1)*col].nLD;
	//回溯
	TraceBack(lpLdData, col-1, row-1, col, lpIndexS, lpIndexT);
	free(lpLdData);
	lpLdData=NULL;
	return result;
}
