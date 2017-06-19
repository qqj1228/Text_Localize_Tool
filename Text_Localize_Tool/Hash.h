#pragma once

static int prime_array[] = {
	17,             /* 0 */
	37,             /* 1 */
	79,             /* 2 */
	163,            /* 3 */
	331,            /* 4 */
	673,            /* 5 */
	1361,           /* 6 */
	2729,           /* 7 */
	5471,           /* 8 */
	10949,          /* 9 */
	21911,          /* 10 */
	43853,          /* 11 */
	87719,          /* 12 */
	175447,         /* 13 */
	350899,         /* 14 */
	701819,         /* 15 */
	1403641,        /* 16 */
	2807303,        /* 17 */
	5614657,        /* 18 */
	11229331,       /* 19 */
	22458671,       /* 20 */
	44917381,       /* 21 */
	89834777,       /* 22 */
	179669557,      /* 23 */
	359339171,      /* 24 */
	718678369,      /* 25 */
	1437356741,     /* 26 */
	2147483647      /* 27 (largest signed int prime) */
}; 

#define TABLESIZE 673

typedef struct tagCTRLHASH
{
	TCHAR tcsCtrl[OUTCTRLLEN];
	int nCtrlLen;
} CTRLHASH, *LPCTRLHASH;
CTRLHASH CtrlHash[TABLESIZE];

inline UINT HashTimes33(LPCTSTR key)
{
	UINT nHash = 0;
	while (*key)
		nHash = (nHash<<5) + nHash + *key++;
	return nHash;
}

bool BuildHashTable(LPCTSTR lpctStr, int nCtrlLen)
{
	memset(CtrlHash, -1, TABLESIZE*sizeof(CTRLHASH));//³õÊ¼»¯¹þÏ£±í
	UINT nPos=HashTimes33(lpctStr) % TABLESIZE;
	if (-1==CtrlHash[nPos].nCtrlLen)
	{
		_tcscpy_s(CtrlHash[nPos].tcsCtrl, OUTCTRLLEN, lpctStr);
		CtrlHash[nPos].nCtrlLen=nCtrlLen;
	}
	else
	{
		UINT i;
		for (i=nPos+1; i!=nPos && -1!=CtrlHash[i].nCtrlLen; i=(++i)%TABLESIZE)
			;
		if(i==nPos)
			return false;
		else
		{
			_tcscpy_s(CtrlHash[nPos].tcsCtrl, OUTCTRLLEN, lpctStr);
			CtrlHash[nPos].nCtrlLen=nCtrlLen;
		}
	}
	return true;
}

int GetHashValue(LPCTSTR lpctStr)
{
	UINT nPos=HashTimes33(lpctStr) % TABLESIZE;
	if (0==_tcscmp(CtrlHash[nPos].tcsCtrl, lpctStr))
		return CtrlHash[nPos].nCtrlLen;
	else
	{
		UINT i;
		for (i=nPos+1; i!=nPos && 0!=_tcscmp(CtrlHash[nPos].tcsCtrl, lpctStr); i=(++i)%TABLESIZE)
			;
		if(i==nPos)
			return 0;
		else
			return CtrlHash[nPos].nCtrlLen;
	}
}