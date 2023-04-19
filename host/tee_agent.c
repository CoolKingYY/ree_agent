#include <linux/acpi.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/tee_drv.h>
#include <linux/tpm.h>
#include <linux/uuid.h>
#include <linux/module.h>

#include "tee_private.h"

#define MAX_SYSCALL_NUM		451  
#define MAX_COMMAND_SIZE       4096
#define MAX_RESPONSE_SIZE      4096

#define TA_HELLO_WORLD_CMD_INC_VALUE		0
#define TA_HELLO_WORLD_CMD_DEC_VALUE		1

static const uuid_t hello_ta_uuid =
	UUID_INIT(0x8aaaf200, 0x2450, 0x11e4, 
		 0xab, 0xe2, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b);

struct mem_critical_data{
	bool MMU;
	u32 PageRW;
	u32 IDT[32];
	u32 SELinux;
	u64 Syscall_Table[MAX_SYSCALL_NUM];
};

static int ftpm_tee_match(struct tee_ioctl_version_data *ver, const void *data)
{
	/*
	 * Currently this driver only support GP Complaint OPTEE based fTPM TA
	 */
	if ((ver->impl_id == TEE_IMPL_ID_OPTEE) &&
		(ver->gen_caps & TEE_GEN_CAP_GP))
		return 1;
	else
		return 0;
}

int  tee_agent(struct mem_critical_data *buffer)
{
    int rc;
    struct tee_context *ctx;
    struct tee_ioctl_open_session_arg sess_arg;
    struct tee_shm *shm;

    ctx = tee_client_open_context(NULL, ftpm_tee_match, NULL,NULL);
	if (IS_ERR(ctx)) {
		if (PTR_ERR(ctx) == -ENOENT)
			return -EPROBE_DEFER;
		printk("tee_client_open_context failed\n");
		return PTR_ERR(ctx);
	}
printk("debug yzc tee   context \n");

    memset(&sess_arg, 0, sizeof(sess_arg));
    export_uuid(sess_arg.uuid, &hello_ta_uuid);
    rc = tee_client_open_session(ctx, &sess_arg, NULL);
	if ((rc < 0) || (sess_arg.ret != 0)) {
		printk(" tee_client_open_session failed, err=%x\n",sess_arg.ret);
		rc = -EINVAL;
	}  
printk("debug yzc tee   session ,rc = %d\n", rc);

    shm = tee_shm_alloc_kernel_buf(ctx,
						 MAX_COMMAND_SIZE +MAX_RESPONSE_SIZE);   //调tee_shm_alloc_helper
    if (IS_ERR(shm)) {
		printk("tee_shm_alloc_kernel_buf failed\n");
		rc = -ENOMEM;
	}
printk("debug yzc tee   shm \n");  //后面chip_register,进入ope_send

    struct tee_ioctl_invoke_arg transceive_args;
    struct tee_param command_params[4];
    memset(&transceive_args, 0, sizeof(transceive_args));
	memset(command_params, 0, sizeof(command_params));

    /* Invoke FTPM_OPTEE_TA_SUBMIT_COMMAND function of fTPM TA */
    transceive_args = (struct tee_ioctl_invoke_arg) {
		.func = TA_HELLO_WORLD_CMD_INC_VALUE,
		.session = sess_arg.session,
		.num_params = 4,
	};

    /* Fill FTPM_OPTEE_TA_SUBMIT_COMMAND parameters */
	command_params[0] = (struct tee_param) {   //共享内存在这里
		.attr = (uint32_t) TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT,
		.u.memref = {
			.shm = shm,
			.size = MAX_COMMAND_SIZE + MAX_RESPONSE_SIZE,
			.shm_offs = 0,
		},
	};

    
    struct mem_critical_data *mem_data = (struct mem_critical_data *)tee_shm_get_va(shm, 0);   //mem_data从shm中获取一段空间的首地址,相当于malloc
	if (IS_ERR(mem_data)) {
		printk("tee_shm_get_va failed for transmit\n");
		return PTR_ERR(mem_data);
	}
	memset(mem_data, 0, (MAX_COMMAND_SIZE + MAX_RESPONSE_SIZE));
	mem_data->MMU = buffer->MMU;
	//printk("mem_data : 0x%lx", mem_data->MMU);

printk("debug yzc shm_get_va, mem_data_address:0x%lx, mem_data_size:%d, attr:%d\n", mem_data, MAX_COMMAND_SIZE + MAX_RESPONSE_SIZE,TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT);

    // memset((void *)shm, 0, sizeof(*shm));  
    // shm->kaddr = &a;
    // shm->size = sizeof(a);
    // shm->flags = TEE_SHM_DYNAMIC;
    // // int test;
    // // memcpy(&test, shm->kaddr, sizeof(test));
    // // printk("test: %d, shm->kaddr %p\n", test, shm->kaddr);
    // struct tee_shm *shmr = tee_shm_register_kernel_buf(ctx, shm, sizeof(*shm));
    // shmr->kaddr = &b;
    // shmr->size = sizeof(b);
    // //  int testr;
    // // memcpy(&testr, shmr->kaddr, sizeof(testr));
    // // printk("testr: %d, shmr->kaddr %p\n", testr, shmr->kaddr);
    // // printk("test: %d, shm->kaddr %p\n", test, shm->kaddr);

printk("MMU Status pre:%d\n", mem_data->MMU);
    rc = tee_client_invoke_func(ctx, &transceive_args,command_params);
printk("MMU Status after:%d\n", mem_data->MMU);
	if ((rc < 0) || (transceive_args.ret != 0)) {
		printk("SUBMIT_COMMAND invoke error: 0x%x\n",transceive_args.ret);
		return (rc < 0) ? rc : transceive_args.ret;
	}

	

	/* Free the shared memory pool */
	tee_shm_free(shm);

	/* close the existing session with fTPM TA*/
	tee_client_close_session(ctx, sess_arg.session);
	/* close the context with TEE driver */
	tee_client_close_context(ctx);
    return 0;
}
EXPORT_SYMBOL(tee_agent);

static int __init agent_init(void)
{
	struct mem_critical_data buffer;
	memset(&buffer, 0, sizeof(buffer));
	buffer.MMU = 0;

	tee_agent(&buffer);
	return 0;
}

static void __exit agent_exit(void)
{

}

module_init(agent_init);
module_exit(agent_exit);

MODULE_AUTHOR("yzc");
MODULE_DESCRIPTION("Agent");
MODULE_LICENSE("GPL v2");
