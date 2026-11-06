extern int arm_fn(void),thumb_fn(void),owned_data[];
extern unsigned char owned_bss[];
int fixture_use(unsigned k) {return arm_fn()+thumb_fn()+owned_data[k&3]+owned_bss[k&8191];}
void *fixture_pointer(void) {return (void *)thumb_fn;}
void *abs_arm=(void *)arm_fn,*abs_thumb=(void *)thumb_fn,*abs_object=(void *)owned_data;
extern int class_vtable[] __asm__("_ZTVN10__cxxabiv117__class_type_infoE");
extern int single_vtable[] __asm__("_ZTVN10__cxxabiv120__si_class_type_infoE");
void *abs_class_vtable=(char *)class_vtable+8,*abs_single_vtable=(char *)single_vtable+8;
__attribute__((weak)) int self_arm(void) {return 3;}
extern int self_thumb(void);
__attribute__((weak)) int self_data[4]={3,4,5,6};
void *abs_self_arm=(void *)self_arm,*abs_self_thumb=(void *)self_thumb,*abs_self_data=(void *)self_data;
int self_calls(unsigned k) {return self_arm()+self_thumb()+self_data[k&3];}
extern void _ITM_registerTMCloneTable(void) __attribute__((weak));
extern void _ITM_deregisterTMCloneTable(void) __attribute__((weak));
extern void __gmon_start__(void) __attribute__((weak));
void (*crt_register_address(void))(void) {return _ITM_registerTMCloneTable;}
void (*crt_deregister_address(void))(void) {return _ITM_deregisterTMCloneTable;}
// Emit both the GOT and PLT forms. This owned fixture is never loaded/executed.
void crt_guarded_call(void) {if(__gmon_start__)__gmon_start__();}
