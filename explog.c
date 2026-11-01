#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>


#define padding widh%4
#define maxfilepathlen 4095
#define NM  "\x1B[0m"
#define RD  "\x1B[31m"
#define GN  "\x1B[32m"
#define YL  "\x1B[33m"
#define BU  "\x1B[34m"
#define MG  "\x1B[35m"
#define CN  "\x1B[36m"
#define WT  "\x1B[37m"
#define BL  "\x1B[30m"

#pragma pack (push, 1)


void printHelp(){
	printf("getopt example\n");
	printf("-f <--filter>\n");
	printf("-c <--change_color>\n");
	printf("-d <--devide_image>");

	printf("-s <--save>\n");
	printf("-e <--end>\n");
	printf("-h -? <--help>\n");

}


typedef struct
{
	unsigned short signature;
	unsigned int filesize;
	unsigned short reserved1;
	unsigned short reserved2;
	unsigned int pixelArrOffset;
} BitmapFileHeader;

typedef struct
{
	unsigned int headerSize;
	unsigned int width;
	unsigned int height;
	unsigned short planes;
	unsigned short bitsPerPixel;
	unsigned int compression;
	unsigned int imageSize;
	unsigned int xPixelsPerMeter;
	unsigned int yPixelsPerMeter;
	unsigned int colorsInColorTable;
	unsigned int importantColorCount;
} BitmapInfoHeader;

typedef struct
{
	unsigned char b;
	unsigned char g;
	unsigned char r;
} rgb;

typedef struct {
	BitmapFileHeader head;
	BitmapInfoHeader info;
	rgb** map;
} bmpfile;

 #pragma pack(pop)


//
void printInfo(bmpfile raw) {
	BitmapFileHeader h = raw.head;
	BitmapInfoHeader header = raw.info;
	printf("signature:\t%x (%hu)\n", h.signature, h.signature);
	printf("filesize:\t%x (%u)\n", h.filesize, h.filesize);
	printf("reserved1:\t%x (%hu)\n", h.reserved1, h.reserved1);
	printf("reserved2:\t%x (%hu)\n", h.reserved2, h.reserved2);
	printf("pixelArrOffset:\t%x (%u)\n", h.pixelArrOffset, h.pixelArrOffset);
	printf("headerSize:\t%x (%u)\n", header.headerSize, header.headerSize);
	printf("width:     \t%x (%u)\n", header.width, header.width);
	printf("height:    \t%x (%u)\n", header.height, header.height);
	printf("planes:    \t%x (%hu)\n", header.planes, header.planes);
	printf("bitsPerPixel:\t%x (%hu)\n", header.bitsPerPixel, header.bitsPerPixel);
	printf("compression:\t%x (%u)\n", header.compression, header.compression);
	printf("imageSize:\t%x (%u)\n", header.imageSize, header.imageSize);
	printf("xPixelsPerMeter:\t%x (%u)\n", header.xPixelsPerMeter, header.xPixelsPerMeter);
	printf("yPixelsPerMeter:\t%x (%u)\n", header.yPixelsPerMeter, header.yPixelsPerMeter);
	printf("colorsInColorTable:\t%x (%u)\n", header.colorsInColorTable, header.colorsInColorTable);
	printf("importantColorCount:\t%x (%u)\n", header.importantColorCount, header.importantColorCount);
}

//
int read_infoheader(FILE* file, BitmapInfoHeader* bmif) {
	fread(&bmif -> headerSize,1, sizeof(int),file);
	if (bmif->headerSize != 12 && bmif->headerSize != 40 && bmif->headerSize != 52 &&
		bmif->headerSize != 56 && bmif->headerSize != 108 && bmif->headerSize != 124) {
		return -1;
	}
	if (bmif->headerSize >= 12) {
		fread(&bmif->width, 1, sizeof(unsigned int), file);
		fread(&bmif->height, 1, sizeof(unsigned int), file);
		fread(&bmif->planes, 1, sizeof(unsigned short), file);
		fread(&bmif->bitsPerPixel, 1, sizeof(unsigned short), file);
	}
	if (bmif->bitsPerPixel != 24) return -2;

	// bmp v1
	if (bmif->headerSize >= 40) {
		fread(&bmif->compression, 1, sizeof(unsigned int), file);
		fread(&bmif->imageSize, 1, sizeof(unsigned int), file);
		fread(&bmif->xPixelsPerMeter, 1, sizeof(unsigned int), file);
		fread(&bmif->yPixelsPerMeter, 1, sizeof(unsigned int), file);
		fread(&bmif->colorsInColorTable, 1, sizeof(unsigned int), file);
		fread(&bmif->importantColorCount, 1, sizeof(unsigned int), file);
	}

	if (bmif->compression != 0) {
		return -3;
	}
	return 0;
	
}

int readbmp(char* name, bmpfile* res) {
	FILE* file = fopen(name, "r");
	if (file == NULL) return -4;

	BitmapFileHeader bmfh;
	BitmapInfoHeader bmif;
	fread(&bmfh, 1, sizeof(BitmapFileHeader), file);
	if (bmfh.signature != 0x4D42) return -5; // 

	int mis = read_infoheader(file, &bmif);
	if (mis) return mis; // 
	unsigned int height = bmif.height;
	unsigned int widh = bmif.width;
	rgb** arr = calloc(height * sizeof(rgb*),1);
	for (int i = 0; i < height; i++) {
		arr[i] = calloc(widh * sizeof(rgb) + padding, 1);
		fread(arr[i], 1, widh * sizeof(rgb) + padding, file);
	}
	res->head = bmfh;
	res->info = bmif;
	res->map = arr;
	return 0;
	
}


int savebmp(bmpfile raw, char* name) {
	FILE* file = fopen(name, "wb");
	if (!file)return -1; 
	int widh = raw.info.width;
	int H = raw.info.height;
	fwrite(&raw.head, 1, sizeof(BitmapFileHeader), file);
	fwrite(&raw.info, 1, sizeof(BitmapInfoHeader), file);
	unsigned int w = widh * sizeof(rgb) + padding;
	for (int i = 0; i < H; i++) {
		fwrite(raw.map[i], 1, w, file);
	}

	fclose(file);
	return 0;
}

rgb make_color(int r, int g, int b) {
	rgb res = { b, g, r };
	return res;
}

int RGB_comp(rgb l1, rgb l2) {
	return((l1.r == l2.r) && (l1.g == l2.g) && (l1.b == l2.b));
}

void RGB_filter(bmpfile* raw, char el, int n) {
	int W = raw->info.width;
	int H = raw->info.height;
	rgb** map = raw->map;
	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {
			if (el == 'R')map[i][j].r = n;
			else if (el == 'G')map[i][j].g = n;
			else map[i][j].b = n;
		}
	}
	raw->map = map;
	
}

rgb color_select(char el) {
	printf("%c\n",el);
	rgb b = { 255, 0, 0 };
	rgb g = { 0, 255, 0 };
	rgb r = { 0, 0, 255 };
	rgb c = { 255, 255, 0 };
	rgb m = { 255, 0, 255 };
	rgb y = { 0, 255, 255 };
	rgb d = { 0, 0, 0 };                          
	rgb w = { 255, 255, 255 };
	rgb def = {1, 1, 1};
	switch(el){
	case 'r':return r;
	case 'g':return g;
	case 'b':return b;
	case 'y':return y;
	case 'm':return m;
	case 'c':return c;
	case 'd':return d;
	case 'w':return w;
	default:
		printf("\nwrong input\n");
		return def;
	}
}

void collorhelp(){
	puts("help to know the color");
	puts("choose the char of color you want to choose");
	printf("%sr to choose red;      ", RD);
	printf("%sg to choose green;\n", GN);
	printf("%sb to choose blue;     ", BU);
	printf("%sy to choose yellow;\n", YL);
	printf("%sm to choose magenta;  ", MG);
	printf("%sc to choose cian;\n", CN);
	printf("%sw to choose white;    ", WT);
	printf("%sd to choose (dark)black%s;", BL,NM);
}

long RGB_swap_color(bmpfile* raw, rgb subj, rgb obj, char* name) {
	int W = raw->info.width;
	int H = raw->info.height;
	rgb** map = raw->map;
	long count = 0;
	for (int i = 0; i < H; i++) {
		for (int j = 0; j < W; j++) {
			if (RGB_comp(map[i][j], subj)) { map[i][j] = obj; count++; }
		}
	}
	raw->map = map;
	savebmp(*raw, name);
	return count;
}
//
//
char* file_name_costruct(int x, int y, char* way) {
	int namelen = 6;
	char name[namelen];
	int len = 0;
	char* dict;
	if (way) {
		dict = calloc(strlen(way) + namelen, sizeof(char));
		strcpy(dict, way);
		len = strlen(way);
	}
	else dict = calloc(namelen, sizeof(char));
	if (len > 0 && way[len] != '/' && way != NULL)name[0] = '/';
	else name[0] = 'Z';
	name[3] = (x % 25) + 65;
	name[4] = (y % 25) + 65;
	name[5] = '\0';
	name[1] = ((x / 25) % 25) + 65;
	name[2] = ((y / 25) % 25) + 65;
 	for (int i = 0; i < 6; i++) {
		dict[len+i] = name[i];
	}
	char* end = ".bmp\0";
	strcat(dict, end);
	return dict;
}

void multi_image(bmpfile raw, int X, int Y, char* way) {
	int w = raw.info.width;
	int h = raw.info.height;
	w = w - w % X; h = h - h % Y; // m*n
	int brick_w = w / X; int brick_h = h / Y;

	rgb** map = raw.map;
	bmpfile res = raw;

	res.info.height = brick_h;
	res.info.width = brick_w;
	res.info.imageSize = (brick_h * brick_w);
	int widh = brick_w;
	rgb** tmp = calloc(brick_h, sizeof(rgb*));
	for (int i = 0; i < brick_h; i++) {
		tmp[i] = calloc(brick_w*sizeof(rgb) + padding, 1);
	}
	int namex = -1;
	int namey = -1;
	for (int I = h - brick_h; I >= 0; I -= brick_h) {
		namey++;
		namex = -1;
		for (int J = 0; J < w; J += brick_w) {
			namex++;
			for (int i = 0; i < brick_h; i++) {
				for (int j = 0; j < brick_w; j++) {
					tmp[i][j] = map[I+i][J+j];

				}
			}
			res.map = tmp;
			char* road =  file_name_costruct(namey, namex, way);
			savebmp(res, road);
		}
	}
	for (int i = 0; i < brick_h; i++){
			free(tmp[i]);
			}
			free(tmp);
}

char* path_read(){
	int flag_s = 1;
	int buf;
	char* path;
	char ch = ' ';
	int len = 0;
	while(flag_s){
		buf = 2;
		path = calloc(buf, sizeof(char));
		while (ch!='\n'){
			scanf("%c", &ch);
			if (len > buf-1){
				buf*=2;
				path = realloc(path, buf);

			}
			if (buf == 8192){
				puts("it seems your path is too long");
				puts("try again please");
				flag_s = 0;
				free(path);
				break;
			}
			if (ch == '\n'){
				flag_s = 0;
				break;
			}
			path[len] = ch;
			len++;
		}
	}
	path[strlen(path)] = '\0';
	return path;
}

int sayhello(bmpfile* raw){
	puts("hello!");
	puts("input the name of file to edit");
	puts("or");
	puts("input the file path:");
	char* path;
	char trash = ' ';
	path = path_read();
	int flag_read = 0;
	flag_read = readbmp(path, raw);
	int attempt_count = 0;
	char t = ' '; 
	while (flag_read){
		printf("\n");
		attempt_count++;
		switch(flag_read){
			case -1:
				puts("wrong version");
				break;
			case -3:
				puts("мы договаривались без сжатия");
				break;
			case -2:
				puts("wrong bitsPerPixel");
				break;
			case -4:
				puts("wrong name or path");
				break;
			case -5:
				puts("it's not bmp format file");
				break;
		}
		if (attempt_count % 5 == 0){
			puts(" ");
			puts("are you okey?");
			puts(" ");
		}

			yn_select(&t);
			if (t == 'n'){
				puts("okey....");
				puts("so");
				puts("..");
				puts("good good bye!");
				return -1;
			}
			if (t == 'y'){

				puts("so try input");
				path = path_read();
				flag_read = readbmp(path, raw);
			}
		}
	free(path);
	return 0;
	}


	// -1 версия
	// -2 бит на пиксель
	// -3 сжатие
	// -4 открытие файла
	// -5 это не bmp

	int component_approove(char mp, int temp){
		int flag_c = 1;
		char c = 0;
		char t;
		char trash;
		while(temp > 255 || temp < 0){
			puts("u made mistake..");
			puts("component num shuld be [0..255]");
			printf("continue with default? %c = 100?\n", mp);
			while (t != 'y' || t != 'n'){
				printf("y/n:\n");
				t = getchar();
				while(trash != '\n') trash = getchar(); 
				trash = ' ';
				if (t == 'y') return 100;
				else{
					puts("please input the [0..255] number");
					scanf("%d", &temp);
					break;
				}
			}
		}
	return temp;	
	}

void yn_select(char* t){
	char trash = ' ';
	while (*t != 'y' || *t != 'n'){
		puts("try again?");
		printf("y/n:\n");
		*t = getchar();
		while(trash != '\n') trash = getchar(); 
		trash = ' ';
		if (*t == 'n' || *t== 'y')break;
	}
}

int choose_color(rgb* clr){
				puts("input the (r, g, b) form of color ex. 200, 0, 255");
				puts("or just press enter to choose one of 7 defoult colors on next step");
				rgb def = {1, 1, 1};
				char t = ' ';
				char* sep = ",. ;:";
				rgb color;
			    color = *clr;
				char* temp = calloc(15 ,sizeof(char));
				fgets(temp, 15, stdin);
				if (temp[0] == '\n'){
					free(temp);
					puts("okey look at this:");
					collorhelp();
					printf("\n");
					puts("input key: ");
					scanf("%c", &t);
					*clr = color_select(t);
					if (1 == color.r == color.g == color.b){ 
						puts("wrong key");
						yn_select(&t);
						if (t == 'n') {color = def; return 0;}
						else choose_color(clr);  
						}
				}
				else{
					char* r = strtok(temp, sep);
					char* g = strtok(NULL, sep);
					char* b = strtok(NULL, sep);
					free(temp);
					if (r && b && g){
						clr->r = atoi(r);
						clr->g = atoi(g);
						clr->b = atoi(b);
						return 0;
					}
					else{
						puts("wrong color components");
						yn_select(&t); 
						if (t == 'n'){color = def; return 0;}
						else return choose_color(clr);	
					}
			}
		
}



int main(int c, char** v){
	bmpfile res;
	int flag_r = sayhello(&res);
	if (flag_r) return 0;

	int component = 100;//default
	int Xcount = 1; int Ycount = 1; //default
	int X = 2; int Y = 1; //default
	
	char t; char trash;
	int optindex;

	char* name_to_save = "out.bmp\0"; //default
	char* path_to_save = NULL;
	char* sep = ";,.";
	FILE* try;
	struct option longOpts[] = {
		{"change_color", no_argument, 0, 'c'},
		{"devide_image", required_argument, 0, 'd'},
		{"name", no_argument,0, 'n'},
		{"Red", required_argument, 0, 'R'},
		{"Green", required_argument, 0, 'G'},
		{"Blue", required_argument, 0, 'B'},
		{"info", no_argument,0, 'i'},
		{"path", no_argument, 0, 'p'},
		{0, 0, 0, 0},
	};
	char opts[] = "fced:S:R:G:B:pnih?";
	int opchar = getopt_long(c, v, opts, longOpts, &optindex);
	while(opchar != -1){
		switch(opchar){
			case 'c':
				rgb color1;
				rgb color2;
				puts("\nchange color");
				puts("lets find the color you want to change");
			    choose_color(&color1);
				if (color1.r == 1 && color1.g == 1 && color1.b == 1){puts("quit"); return 0;}
				printf("\n%d, %d, %d\n", color1.r, color1.g, color1.b);
				puts("succes\n");
				getchar();

				puts("lets find the color you want to be changed");
				choose_color(&color2);
				if (1 == color2.r == color2.g == color2.b){puts("quit"); return 0;};
				printf("\n%d, %d, %d\n", color2.r, color2.g, color2.b);
				puts("succes\n");

				puts("change_color");
				int ct = RGB_swap_color(&res, color1, color2, name_to_save);
				printf("colors changed:= %d\n", ct);
				break;
			case 'd':
				char* XY = optarg;
				char* sY = strtok(XY, sep);
				char* sX = strtok(NULL, sep);
				if(sX && sY){
					X = atoi(sX);
					Y = atoi(sY);
					if (X > res.info.width && Y > res.info.height){
						puts("to big parametrs for devide");
						puts("sorry");
						return 0;
					}
					for (int i = 0; i< strlen(sX); i++) if (!isdigit(sX[i])) X = 0;
					for (int i = 0; i< strlen(sY); i++) if (!isdigit(sY[i])) Y = 0;
					if (X == 0){
						puts("you made mistake so i set n:=2");
						X = 2;
					}
					if (Y == 0){
						puts("you made mistake so i set m:=1");
						Y = 1;
					}
				}

				else{
					puts("wrong parametrs for devide");
					puts("sorry");
					return 0; 
				}

				puts("devide_image\n");
				multi_image(res, X, Y, path_to_save);
				puts("succes");
				break;
			case 'R':
			case 'G':
			case 'B':
			 	printf("%c\n", opchar);
				component = component_approove(opchar, atoi(optarg));
				RGB_filter(&res, opchar, component);
				savebmp(res, name_to_save);
				printf("component %c = %d\n", opchar, component);
				puts("succes");
				break;
			case 'p':
				puts("please input path");
				path_to_save = path_read();
				char* pathtemp = calloc(strlen(path_to_save), sizeof(char));
				strcpy(pathtemp, path_to_save);
				pathtemp = realloc(pathtemp, (strlen(pathtemp)+strlen(name_to_save))*sizeof(char));
				if (pathtemp[strlen(pathtemp)] != '/'){
					pathtemp[strlen(pathtemp)] = '/';
				}
				pathtemp[strlen(pathtemp)] = '\0';
				strcat(pathtemp, name_to_save);

				FILE* try = fopen(pathtemp, "wr");
				while(try == NULL){
					puts("wrong path");
					yn_select(&t);
					if (t == 'n'){
						puts("okey set default no path");
						path_to_save = NULL;
						break;
					}
					puts("so try input");
					path_to_save = path_read();
					strcpy(pathtemp, path_to_save);

					if (pathtemp[strlen(path_to_save)] != '/')
						pathtemp[strlen(path_to_save)] = '/';

					pathtemp = realloc(pathtemp, (strlen(pathtemp)+strlen(name_to_save))*sizeof(char));
					strcat(pathtemp, name_to_save);
					try = fopen(pathtemp, "wr");
				}
				if (try)fclose(try);
				if (path_to_save) name_to_save = pathtemp;
				break;
			case 'n':
				puts("please input name");
				name_to_save = path_read();
				puts(name_to_save);
				try = fopen(name_to_save, "wr");
				while(try == NULL){
					puts("wrong name");
					yn_select(&t);
					if (t == 'n'){
						puts("okey set default name \"out.bmp\"");
						name_to_save = "out.bmp";
						break;
					}
					puts("so try input");
					name_to_save = path_read();
					try = fopen(name_to_save, "r");
				}
				name_to_save[strlen(name_to_save)] = '\0';
				fclose(try);
				break;

			case 'i':
				printInfo(res);
				break;
			case '?':
			case 'h':
				printHelp();
				break;
			default:
				puts("get the fuck out");
				printf("optchar: %s\n", optarg);
				break;
		}

		opchar = getopt_long(c, v, opts, longOpts, &optindex);
	}
	for (int i = 0; i < res.info.height; i++){
			free(res.map[i]);
			}
			free(res.map);
	return 0;
}