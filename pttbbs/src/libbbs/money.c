#include <stdio.h>

#include <libbbs.h>

/* �p���ػP�| */
int
give_tax(int money)
{
    int             i, tax = 0;
    int      tax_bound[] = {1000000, 100000, 10000, 1000, 0};
    double   tax_rate[] = {0.4, 0.3, 0.2, 0.1, 0.08};
    for (i = 0; i <= 4; i++)
	if (money > tax_bound[i]) {
	    tax += (money - tax_bound[i]) * tax_rate[i];
	    money -= (money - tax_bound[i]);
	}
    return (tax <= 0) ? 1 : tax;
}

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

