extern int owned_data[];
extern unsigned char owned_bss[];
int main(void) {return owned_data[1]+owned_bss[0];}
