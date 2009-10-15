#include <stdio.h>
#include <assert.h>
#include "cmbbs.h"

const char*
money_level(int money)
{
    int i = 0;

    static const char *money_msg[] =
    {
	"�ťx���v", "���h", "�M�H", "���q", "�p�d",
	"�p�I", "���I", "�j�I��", "�I�i�İ�", "�񺸻\\��", NULL
    };
    while (money_msg[i] && money > 10)
	i++, money /= 10;

    if(!money_msg[i])
	i--;
    return money_msg[i];
}

