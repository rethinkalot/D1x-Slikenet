#ifndef _VMS_TYPES_H_GUARD
#define _VMS_TYPES_H_GUARD
#include <stdint.h>
#ifndef __pack__
#define __pack__ __attribute__((packed))
#endif

typedef int32_t fix;
typedef int16_t fixang;

#pragma pack(push, 1)
typedef struct vms_vector {
    union {
        struct { fix x, y, z; };
        fix xyz[3];
    };
} __pack__ vms_vector;

typedef union vms_vector_array {
    struct { fix x, y, z; };
    fix xyz[3];
} __pack__ vms_vector_array;

typedef struct vms_svec {
    union {
        struct { int16_t x, y, z; };
        int16_t xyz[3];
    };
} __pack__ vms_svec;
#pragma pack(pop)

typedef struct vms_angvec { fixang p, b, h; } __pack__ vms_angvec;
typedef struct vms_matrix { vms_vector rvec; vms_vector uvec; vms_vector fvec; } __pack__ vms_matrix;
typedef struct vms_quaternion { fix x, y, z, w; } __pack__ vms_quaternion;
#endif
