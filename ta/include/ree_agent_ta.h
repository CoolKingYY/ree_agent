#ifndef REE_AGENT_TA_H
#define REE_AGENT_TA_H

#define TA_REE_AGENT_UUID { 0x3b3f9a9d, 0x08d5, 0x4028,\
    { 0xaa, 0xe1, 0x87, 0xcb, 0xed, 0x59, 0xe6, 0xb8 } }



/* The function IDs implemented in this TA */
#define TA_REE_AGENT_CMD_CREATE_PERSISTENT_MEM    1 
#define TA_REE_AGENT_CMD_ADD_REFERENCE     2
#define TA_REE_AGENT_CMD_DEL_REFERENCE     3
#define TA_REE_AGENT_CMD_MATCH_REFERENCE     4
#define TA_REE_AGENT_CMD_JUDGE_REFERENCE     5
#define TA_REE_AGENT_CMD_SM3    6


//struct for kernel CA
#define u8 unsigned char
#define u32 unsigned int
#define u64 unsigned long 
#define MAX_SYSCALL_NUM		451
#define MAX_SHM_SIZE     128 *1024
#define MAX_HASH_NUM		(MAX_SHM_SIZE - 4) / 32  


#define TEE_DATABASE_ADD_BASELINE       0x1 
#define TEE_DATABASE_DEL_BASELINE       0x2 
#define TEE_DATABASE_READ_BASELINE      0x4 
 
#define TEE_DATABASE_ELF        0x1 
#define TEE_DATABASE_SO         0x2 
#define TEE_DATABASE_KO         0x4 
#define TEE_DATABASE_MODULE_TEXT        0x8 
#define TEE_DATABASE_KERNEL_TEXT        0x10 
#define TEE_DATABASE_TASK_TEXT          0x20 
#define TEE_DATABASE_MEM_CRITICAL_DATA          0x40 
 
#define TEE_DATABASE_OP_SUCCESS         0x1 
#define TEE_DATABASE_OP_FAILED         0x2 


typedef struct tee_reference_value
{
    unsigned int length; // 摘要值长度
    const void *value;   // 指向摘要值的指针
} TEEReferenceValue;

struct mem_critical_data{
	bool MMU;
	u32 PageRW;
	u32 IDT[32];
	u32 SELinux;
	u64 Syscall_Table[MAX_SYSCALL_NUM];
};

struct SM3_Baseline{   //4+32 
    //u32 filepath; //文件一级相对路径
    u8 Hash[32];
};

struct TEE_Message{
    u8 operator_type;   //1
    u8 baseline_type;   //1
    int length; //1 
    int result; //1 备用
    struct SM3_Baseline SM3_array[MAX_HASH_NUM];
};


#endif /* REE_AGENT_TA_H_ */