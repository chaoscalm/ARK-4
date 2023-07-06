#include "common.h"
#include <psputility.h>
#include <time.h>

#include "systemctrl.h"
#include "clock.h"

#include "vsh.h"
#include "fonts.h"
#include "advanced.h"

extern int pwidth;
extern char umd_path[72];

extern char device_buf[13];
extern char umdvideo_path[256];

extern int xyPoint[];
extern int xyPoint2[];
extern u32 colors[];

extern vsh_Menu *g_vsh_menu;
int sub_stop_stock = 0;


int item_fcolor[SUBMENU_MAX];
const char *subitem_str[SUBMENU_MAX];
static int submenu_sel = SUBMENU_USB_DEVICE;


int submenu_draw(void) {
	char msg[128] = {0};
	int submax_menu, subcur_menu;
	const int *pointer;
	u32 fc, bc;

	// check & setup video mode
	if (blit_setup() < 0) 
		return -1;
	
	if (pwidth == 720)
		pointer = xyPoint;
	else
		pointer = xyPoint2;

	// show menu title & ARK version
	blit_set_color(0xFFFFFF,0x8000FF00);
	scePaf_snprintf(msg, 128, " %s ", g_messages[MSG_ADVANCED_VSH]);
	blit_string_ctr(pointer[1], msg);
	blit_string_ctr(55, g_vsh_menu->ark_version);
	fc = 0xFFFFFF;
	
	int submenu_start_x, submenu_start_y;
	int window_char, window_pixel;
	int width = 0, temp = 0, i;
	// find widest submenu up until the UMD region option
	for (i = SUBMENU_USB_DEVICE; i <= SUBMENU_UMD_REGION_MODE; i++){
		temp = scePaf_strlen(g_messages[MSG_USB_DEVICE + i]);
		if (temp > width)
			width = temp;
	}
	
	window_char = width + submenu_find_longest_string() + 3;
	if (window_char & 0x1)
		window_char++;
	window_pixel = window_char * 8;
	// submenu width + leading & trailing space + subitem space + subitem width
	submenu_start_x = (pwidth - window_pixel) / 2;
	submenu_start_y = pointer[5] * 8;

	for (submax_menu = 0; submax_menu < SUBMENU_MAX; submax_menu++) {
		// set default colors
		bc = colors[g_vsh_menu->config.ark_menu.vsh_bg_color];
		switch(g_vsh_menu->config.ark_menu.vsh_fg_color){
			case 0: break;
			case 1: fc = colors[27]; break;
			case 27: fc = colors[1]; break;
			default: fc = colors[g_vsh_menu->config.ark_menu.vsh_fg_color]; break;
		}
		
		// add line at the top
		if (submax_menu == 0){
			blit_set_color(fc, bc);
			blit_rect_fill(submenu_start_x, submenu_start_y, window_pixel, 8);
			blit_set_color(0xaf000000, 0xaf000000);
			blit_rect_fill(submenu_start_x, submenu_start_y-1, window_pixel, 1); // top horizontal outline
			blit_rect_fill(submenu_start_x+window_pixel, submenu_start_y, 1, 8*(SUBMENU_MAX+2)); // right vertical outline
			blit_rect_fill(submenu_start_x-1, submenu_start_y, 1, 8*(SUBMENU_MAX+2)); // left vertical outline
		}
		
		// if menu is selected, change color
		if (submax_menu==submenu_sel){
			bc = (g_vsh_menu->config.ark_menu.vsh_bg_color < 2 || g_vsh_menu->config.ark_menu.vsh_bg_color > 28)? 0xff8080:0x0000ff;
			fc = 0xffffff;
			bc |= (((u32)g_vsh_menu->status.bc_alpha)<<24);
			if (g_vsh_menu->status.bc_alpha == 0) g_vsh_menu->status.bc_delta = 5;
			else if (g_vsh_menu->status.bc_alpha == 255) g_vsh_menu->status.bc_delta = -5;
			g_vsh_menu->status.bc_alpha += g_vsh_menu->status.bc_delta;
		}
		
		blit_set_color(fc, bc);

		// display menu
		if (g_messages[MSG_USB_DEVICE + submax_menu]) {
			int len = 0, offset = 0, padding = 0;
			int submenu_width = 0;
			subcur_menu = submax_menu;
			
			// set the y position
			submenu_start_y += 8;
			
			temp = 0;
			// find widest submenu up until the UMD region option
			for (i = SUBMENU_USB_DEVICE; i <= SUBMENU_UMD_REGION_MODE; i++){
				temp = scePaf_strlen(g_messages[MSG_USB_DEVICE + i]);
				if (temp > submenu_width)
					submenu_width = temp;
			}
			
			// submenus between USB_DEVICE and UMD_REGION are the only ones with subitems
			if (submax_menu >= SUBMENU_USB_DEVICE && submax_menu <= SUBMENU_UMD_REGION_MODE) {
				int subitem_start_x;
				
				scePaf_snprintf(msg, 128, " %-*s  ", submenu_width, g_messages[MSG_USB_DEVICE + submax_menu]);
				subitem_start_x = blit_string(submenu_start_x, submenu_start_y, msg);
	
				if(subitem_str[submax_menu]) {
					// check if PSP Go or PSVita because UMD Region mode is unsupported on them
					if ((g_vsh_menu->psp_model == PSP_GO || IS_VITA_ADR(g_vsh_menu->config.p_ark)) && submax_menu == SUBMENU_UMD_REGION_MODE) {
						// write the unsupported string
						scePaf_snprintf(msg, 128, "%-*s", window_char - 3 - submenu_width, g_messages[MSG_UNSUPPORTED]);
					} else {
						// left-justify submenu options that have a subitem next to it
						scePaf_snprintf(msg, 128, "%-*s", window_char - 3 - submenu_width, subitem_str[submax_menu]);
					}
				}
				
				blit_string(subitem_start_x, submenu_start_y, msg);
			// for all other submenus (ie those with no subitems)
			} else {
				// center-justify submenu options
				if (g_vsh_menu->psp_model != PSP_GO && submax_menu == SUBMENU_DELETE_HIBERNATION) {
					// hibernation mode unsupported if model is not PSP Go
					len = scePaf_strlen(g_messages[MSG_NO_HIBERNATION]);
					padding = (window_char - len) / 2;
					scePaf_snprintf(msg, 128, "%*s%s%*s", padding, "", g_messages[MSG_NO_HIBERNATION], padding, "");
				} else {
					len = scePaf_strlen(g_messages[MSG_USB_DEVICE + submax_menu]);
					padding = (window_char - len) / 2;
					scePaf_snprintf(msg, 128, "%*s%s%*s", padding, "", g_messages[MSG_USB_DEVICE + submax_menu], padding, "");
				}
				
				// add a halfspace before if the lenght is an odd value
				if (len & 0x1)
					blit_rect_fill(submenu_start_x, submenu_start_y, 4, 8);
				
				blit_string_ctr(submenu_start_y, msg);
			
				// add a halfspace after if the length is an odd value
				if (len & 0x1) {
					offset = blit_get_string_width(msg);
					blit_rect_fill(submenu_start_x + offset + 4, submenu_start_y, 4, 8);
				}
			}
		}
	}
	
	// set default colors
	bc = colors[g_vsh_menu->config.ark_menu.vsh_bg_color];
	switch(g_vsh_menu->config.ark_menu.vsh_fg_color){
		case 0: break;
		case 1: fc = colors[27]; break;
		case 27: fc = colors[1]; break;
		default: fc = colors[g_vsh_menu->config.ark_menu.vsh_fg_color]; break;
	}

	blit_set_color(fc, bc);
	submenu_start_y += 8; // replace by font width
	// add line at the end
	blit_rect_fill(submenu_start_x, submenu_start_y, window_pixel, 8);
	blit_set_color(0xaf000000, 0xaf000000);
	blit_rect_fill(submenu_start_x, submenu_start_y+8, window_pixel, 1); // bottom horizontal outline
	
	blit_set_color(0x00ffffff,0x00000000);
	return 0;
}

int submenu_find_longest_string(void){
	int width = 0, temp = 0, i;
	temp = scePaf_strlen(g_messages[SUBITEM_DEFAULT]);
	if (temp > width)
		width = temp;
	
	for (i = SUBITEM_REGION; i <= SUBITEM_REGION_END; i++) {
		temp = scePaf_strlen(g_messages[i]);
		if (temp > width)
			width = temp;
	}
	
	for (i = SUBITEM_USBREADONLY; i <= SUBITEM_USBREADONLY_END; i++) {
		temp = scePaf_strlen(g_messages[i]);
		if (temp > width)
			width = temp;
	}
	
	for (i = SUBITEM_SWAPXO; i <= SUBITEM_SWAPXO_END; i++) {
		temp = scePaf_strlen(g_messages[i]);
		if (temp > width)
			width = temp;
	}
	
	for (i = SUBITEM_ISO_DRIVER; i <= SUBITEM_ISO_DRIVER_END; i++) {
		temp = scePaf_strlen(g_messages[i]);
		if (temp > width)
			width = temp;
	}
	
	for (i = SUBITEM_PANDORA; i <= SUBITEM_PANDORA_END; i++) {
		temp = scePaf_strlen(g_messages[i]);
		if (temp > width)
			width = temp;
	}
	
	temp = scePaf_strlen(g_messages[SUBITEM_UNSUPPORTED]);
	if (temp > width)
		width = temp;
	
	for (i = SUBITEM_USBDEVICE; i <= SUBITEM_USBDEVICE_END; i++) {
		temp = scePaf_strlen(g_messages[i]);
		if (temp > width)
			width = temp;
	}
	
	temp = scePaf_strlen(g_messages[SUBITEM_NONE]);
	if (temp > width)
		width = temp;
	
	for (i = SUBITEM_COLOR; i <= SUBITEM_COLOR_END; i++) {
		temp = scePaf_strlen(g_messages[i]);
		if (temp > width)
			width = temp;
	}
	
	return width;
}


int submenu_setup(void) {
	int i;
	const char *bridge;
	const char *umdvideo_disp;

	// preset
	for (i = 0; i < SUBMENU_MAX; i++) {
		subitem_str[i] = NULL;
		item_fcolor[i] = RGB(255,255,255);
	}
	
	//usb device
	if ((g_vsh_menu->config.se.usbdevice > 0) && (g_vsh_menu->config.se.usbdevice < 5)) {
		scePaf_sprintf(device_buf, "%s %d", g_messages[MSG_FLASH], g_vsh_menu->config.se.usbdevice - 1);
		bridge = device_buf;
	} else if (IS_VITA_ADR(g_vsh_menu->config.p_ark)) {
		scePaf_sprintf(device_buf, "%s", g_messages[MSG_USE_ADRENALINE_SETTINGS]);
		bridge = device_buf;
	} else {
		const char *device;
		if(g_vsh_menu->config.se.usbdevice==5)
			device= g_messages[MSG_UMD_DISC];
		else if(g_vsh_menu->psp_model == PSP_GO)
			device = g_messages[MSG_INTERNAL_STORAGE];
		else
			device = g_messages[MSG_MEMORY_STICK];

		bridge = device;
	}
	subitem_str[SUBMENU_USB_DEVICE] = bridge;

	umdvideo_disp = (const char*)scePaf_strrchr(umdvideo_path, '/');

	if (umdvideo_disp == NULL)
		umdvideo_disp = umdvideo_path;
	else
		umdvideo_disp++;

	if (IS_VITA_ADR(g_vsh_menu->config.p_ark))
		subitem_str[SUBMENU_UMD_VIDEO] = g_messages[MSG_UNSUPPORTED];
	else
		subitem_str[SUBMENU_UMD_VIDEO] = umdvideo_disp;

	if (g_vsh_menu->config.se.umdmode == 3)
		subitem_str[SUBMENU_UMD_MODE] = g_messages[MSG_INFERNO];
	else if (g_vsh_menu->config.se.umdmode < 2)
		g_vsh_menu->config.se.umdmode = 3;
	else if (g_vsh_menu->config.se.umdmode > 3)
		g_vsh_menu->config.se.umdmode = 2;
	else 
		subitem_str[SUBMENU_UMD_MODE] = g_messages[MSG_NP9660];

	if (g_vsh_menu->config.ark_menu.vsh_font)
		subitem_str[SUBMENU_FONT] = font_list()[g_vsh_menu->config.ark_menu.vsh_font - 1];
	else
		subitem_str[SUBMENU_FONT] = g_messages[MSG_DEFAULT];

	switch (g_vsh_menu->config.se.usbdevice_rdonly) {
		case 0:
			subitem_str[SUBMENU_USB_READONLY] = g_messages[MSG_DISABLE];
			break;
		case 1:
			subitem_str[SUBMENU_USB_READONLY] = g_messages[MSG_ENABLE];
			break;
		case 2:
			subitem_str[SUBMENU_USB_READONLY] = g_messages[MSG_UNSUPPORTED];
			break;
		default:
			subitem_str[SUBMENU_USB_READONLY] = g_messages[MSG_ENABLE];
	}

	subitem_str[SUBMENU_SWAP_XO_BUTTONS] = g_messages[MSG_O_PRIM-g_vsh_menu->status.swap_xo];

	subitem_str[SUBMENU_CONVERT_BATTERY] = (g_vsh_menu->battery<2)? g_messages[MSG_NORMAL_TO_PANDORA+g_vsh_menu->battery] : g_messages[MSG_UNSUPPORTED];

	if (g_vsh_menu->config.se.vshregion < 14){
		subitem_str[SUBMENU_REGION_MODE] = g_messages[g_vsh_menu->config.se.vshregion];
	}
	else {
		subitem_str[SUBMENU_REGION_MODE] = g_messages[MSG_DISABLE];
	}

	if (g_vsh_menu->config.se.umdregion < 4){
		subitem_str[SUBMENU_UMD_REGION_MODE] = g_messages[g_vsh_menu->config.se.umdregion];
	}
	else {
		subitem_str[SUBMENU_UMD_REGION_MODE] = g_messages[MSG_DEFAULT];
	}
	
	if (g_vsh_menu->config.ark_menu.vsh_fg_color < 29){
		switch(g_vsh_menu->config.ark_menu.vsh_fg_color){
			case 1: subitem_str[SUBMENU_FG_COLORS] = g_messages[MSG_WHITE]; break;
			case 27: subitem_str[SUBMENU_FG_COLORS] = g_messages[MSG_RED]; break;
			default: subitem_str[SUBMENU_FG_COLORS] = g_messages[MSG_RANDOM+g_vsh_menu->config.ark_menu.vsh_fg_color]; break;
		}
		
	}
	else {
		subitem_str[SUBMENU_BG_COLORS] = g_messages[MSG_RED];
	}

	if (g_vsh_menu->config.ark_menu.vsh_bg_color < 29){
		subitem_str[SUBMENU_BG_COLORS] = g_messages[MSG_RANDOM+g_vsh_menu->config.ark_menu.vsh_bg_color];
	}
	else {
		subitem_str[SUBMENU_BG_COLORS] = g_messages[MSG_RED];
	}

	return 0;
}


int submenu_ctrl(u32 button_on) {
	int direction;

	if ((button_on & PSP_CTRL_SELECT) || (button_on & PSP_CTRL_HOME) || button_decline(button_on)) {
		submenu_sel = SUBMENU_GO_BACK;
		return 1;
	}

	// change menu select
	direction = 0;

	if (button_on & PSP_CTRL_DOWN) 
		direction++;
	if (button_on & PSP_CTRL_UP) 
		direction--;

	#define ROLL_OVER(val, min, max) ( ((val) < (min)) ? (max): ((val) > (max)) ? (min) : (val) )
	submenu_sel = ROLL_OVER(submenu_sel + direction, 0, SUBMENU_MAX - 1);

	// LEFT & RIGHT
	direction = -2;

	if(button_on & PSP_CTRL_LEFT)
		direction = -1;
	if(button_accept(button_on))
		direction = 0;
	if(button_on & PSP_CTRL_RIGHT) 
		direction = 1;

	if(direction <= -2)
		return 0;

	switch(submenu_sel) {
		case SUBMENU_USB_DEVICE:
			if (IS_VITA_ADR(g_vsh_menu->config.p_ark)) 
				break;
			if (direction) 
				change_usb(direction);
			break;
		case SUBMENU_USB_READONLY:
			if (IS_VITA_ADR(g_vsh_menu->config.p_ark)) 
				break;
			if (direction) 
				swap_readonly(direction);
			break;
		case SUBMENU_UMD_MODE:
			if (direction) 
				change_umd_mode(direction);
			break;
		case SUBMENU_UMD_VIDEO:
			if (IS_VITA_ADR(g_vsh_menu->config.p_ark)) 
				break;
			if (direction) {
			   	change_umd_mount_idx(direction);

				if(umdvideo_idx != 0) {
					char *umdpath;
					umdpath = umdvideolist_get(&g_umdlist, umdvideo_idx-1);

					if(umdpath != NULL) {
						scePaf_strncpy(umdvideo_path, umdpath, sizeof(umdvideo_path));
						umdvideo_path[sizeof(umdvideo_path)-1] = '\0';
					} else
						goto none;
				} else {
none:
					scePaf_strcpy(umdvideo_path, g_messages[MSG_NONE]);
				}
			} else
				return 6; // Mount UMDVideo ISO flag
			break;
		case SUBMENU_IMPORT_CLASSIC_PLUGINS:
			if (direction == 0)
				return 13; // Import Classic Plugins flag 
			break;
		case SUBMENU_DELETE_HIBERNATION:
			if (direction == 0)
				return 10; // Delete Hibernation flag 
			break;
		case SUBMENU_RANDOM_GAME:
			if (direction == 0)
				return 14; // Random Game flag 
			break;
		case SUBMENU_ACTIVATE_FLASH_WMA:
			if (direction == 0)
				return 11; // Activate Flash/WMA flag 
			break;
		case SUBMENU_REGION_MODE:
			if (direction) 
				change_region(direction, 13);
			break;
		case SUBMENU_UMD_REGION_MODE:
			if (g_vsh_menu->psp_model == PSP_GO || IS_VITA_ADR(g_vsh_menu->config.p_ark)) 
				break;
			if (direction) 
				change_umd_region(direction, 3);
			break;
		case SUBMENU_SWAP_XO_BUTTONS:
			if (direction == 0)
				return 12; // Swap X/O Buttons flag  
			break;
		case SUBMENU_CONVERT_BATTERY:
			if(direction == 0)
				return 9; // Convert Battery flag
			break;
		case SUBMENU_FG_COLORS:
			// This will be where I will be adding to set the color
			if (direction)
				change_fg_color(direction);
			break;
		case SUBMENU_BG_COLORS:
			// This will be where I will be adding to set the color
			if(direction) 
				change_bg_color(direction);
			break;
		case SUBMENU_FONT:
			if (direction) {
				change_font(direction);
				release_font();
				font_load(g_vsh_menu);
			}
			break;
		case SUBMENU_GO_BACK:
			if(direction==0) 
				return 1; // finish
			break;
	}

	return 0; // continue
}


void subbutton_func(vsh_Menu *vsh) {
	int res;
	// submenu control
	switch(vsh->status.submenu_mode) {
		case 0:	
			if ((vsh->buttons.pad.Buttons & ALL_CTRL) == 0)
				vsh->status.submenu_mode = 1;
			break;
		case 1:
			res = submenu_ctrl(vsh->buttons.new_buttons_on);

			if (res != 0) {
				sub_stop_stock = res;
				vsh->status.submenu_mode = 2;
			}
			break;
		case 2: // exit waiting 
			// exit submenu
			if ((vsh->buttons.pad.Buttons & ALL_CTRL) == 0)
				vsh->status.sub_stop_flag = sub_stop_stock;
			break;
	}
}
