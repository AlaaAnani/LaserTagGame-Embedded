#include "gui.h"
#include "stdio.h"

int8_t _health = 100;
int8_t _ammo = 100;
int8_t gunCap = 100;
const int DELAY = 20;

char emptyHeart[] = {0x00, 0x0A, 0x15, 0x11, 0x0A, 0x04, 0x00, 0x00};
char fullHeart[] = {0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00};

void creatHearts()
{
	lcd_create_char(fullHeart, 0);
	lcd_create_char(emptyHeart, 1);
}

void healthBar(uint8_t n)
{
	uint8_t val = (int)(n / 10);
	
	lcd_put_cur(0, 0);
	lcd_send_data(0);
	lcd_put_cur(0, 1);
	lcd_send_data(':');
	
	lcd_put_cur(0, 2);
	lcd_send_string("          ");
	lcd_put_cur(0, 2);
	for (int i = 0; i < val; i++)
		lcd_send_data((char)255);
}

void ammoBar(uint8_t n)
{
	char val[8];
	
	lcd_put_cur(1, 0);
	lcd_send_string("        ");
	lcd_put_cur(1, 0);
	sprintf(val, "AMMO:%i", n);
	lcd_send_string(val);
}

void reloadIndicator()
{
	if (_ammo == 0) {
		lcd_put_cur(1, 10);
		lcd_send_string ("RELOAD");
	} else {
		lcd_put_cur(1, 10);
		lcd_send_string ("      ");
	}
}

void deathIndicator()
{
	if (_health == 0) {
		lcd_put_cur(0, 12);
		lcd_send_string ("DIED");
	} else {
		lcd_put_cur(0, 12);
		lcd_send_string ("    ");
	}
}

void UImain()
{
	creatHearts();
	healthBar(_health);
	ammoBar(_ammo);
	deathIndicator();
	reloadIndicator();
}

void UIhealth(int8_t d)
{
	_health = d;
	if (_health < 0) _health = 0;
	lcd_put_cur(0, 0);
	lcd_send_data(1);
	HAL_Delay(DELAY);
	healthBar(_health);
	deathIndicator();
}

void UIheal()
{
	_health = 100;
	healthBar(_health);
	deathIndicator();
}

void UIammo(int8_t b)
{
	_ammo = b;
	if (_ammo < 0) _ammo = 0;
	ammoBar(_ammo);
	reloadIndicator();
}

void UIreload()
{
	_ammo = gunCap;
	ammoBar(_ammo);
	reloadIndicator();
}
