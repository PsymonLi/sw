#ifndef IONIC_ABI_H
#define IONIC_ABI_H

#include <linux/types.h>

#define IONIC_ABI_VERSION	4

struct ionic_ctx_req {
	__u32 fallback;
	__u32 rsvd;
};

struct ionic_ctx_resp {
	__u32 fallback;
	__u32 page_shift;

	__aligned_u64 dbell_offset;

	__u16 version;
	__u8 qp_opcodes[7];
	__u8 admin_opcodes[7];

	__u8 sq_qtype;
	__u8 rq_qtype;
	__u8 cq_qtype;
	__u8 admin_qtype;
};

struct ionic_qdesc {
	__aligned_u64 addr;
	__u32 size;
	__u16 mask;
	__u8 depth_log2;
	__u8 stride_log2;
};

struct ionic_ah_resp {
	__u32 ahid;
};

struct ionic_cq_req {
	struct ionic_qdesc cq;
	__u8 compat;
	__u8 rsvd[3];
};

struct ionic_cq_resp {
	__u32 cqid;
};

struct ionic_qp_req {
	struct ionic_qdesc sq;
	struct ionic_qdesc rq;
	__u8 compat;
	__u8 rsvd[3];
};

struct ionic_qp_resp {
	__u32 qpid;
	__u32 rsvd;
	__aligned_u64 sq_hbm_offset;
};

#endif /* IONIC_ABI_H */
