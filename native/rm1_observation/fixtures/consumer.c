extern int arm_fn(void),thumb_fn(void),owned_data[];
extern unsigned char owned_bss[];
int fixture_use(unsigned k) {return arm_fn()+thumb_fn()+owned_data[k&3]+owned_bss[k&8191];}
void *fixture_pointer(void) {return (void *)thumb_fn;}
