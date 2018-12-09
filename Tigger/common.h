#ifndef _COMMON_HH_
#define _COMMON_HH_

#include <string>
#include <vector>
/* global variables */
extern int global_debug_flag;

/* data structures */
enum Operator_Type {
        OADD, OSUB, OMUL, ODIV, OMOD, 
        OGT, OLT, OEQ, ONE, OGE, OLE, 
        OAND, OOR, OINC, ODEC, ONOT
};

/* helper functions */
std::vector<std::string> split(const std::string &s, char c);
void Emit(FILE *f, const std::string &str);

/* helper macros */
#define INSTANCE_OF(value, Type) Type * __val = dynamic_cast<Type*>(value)

#define KNRM  "\x1B[0;0m"
#define KRED  "\x1B[0;31m"
#define KGRN  "\x1B[0;32m"
#define KYEL  "\x1B[0;33m"
#define KBLU  "\x1B[0;34m"
#define KMAG  "\x1B[0;35m"
#define KCYN  "\x1B[0;36m"
#define KWHT  "\x1B[0;37m"
#define BOLD_KNRM  "\x1B[1;30m"
#define BOLD_KRED  "\x1B[1;31m"
#define BOLD_KGRN  "\x1B[1;32m"
#define BOLD_KYEL  "\x1B[1;33m"
#define BOLD_KBLU  "\x1B[1;34m"
#define BOLD_KMAG  "\x1B[1;35m"
#define BOLD_KCYN  "\x1B[1;36m"
#define BOLD_KWHT  "\x1B[1;37m"


#endif
