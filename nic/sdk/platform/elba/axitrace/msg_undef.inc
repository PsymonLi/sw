#include <iostream>
#include "generic.h"

#ifdef MY_MSG
#undef MY_MSG
#undef MY_INFO
#undef MY_ERROR
#undef MY_WARNING
#undef MY_DEBUG
#endif
