// <errno.h> da libks (doc 05 §2).
//
// errno é a global ks_errno, escrita pelos wrappers de syscall
// (libks/asm/syscall.ksasm) e pelas funções da libks. Os códigos de 1 a 6
// são os próprios KSVM_ERR_* devolvidos pela VM em r2 (doc 06 §3 da
// ksvm2), então não há tabela de tradução; os demais são da libks.
#ifndef __ERRNO_H
#define __ERRNO_H

extern int ks_errno;
#define errno ks_errno

#define ENOSYS 1   // KSVM_ERR_NOSYS: syscall ou serviço ausente na HAL
#define EFAULT 2   // KSVM_ERR_FAULT: ponteiro/faixa inválida
#define EINVAL 3   // KSVM_ERR_INVAL: argumento inválido
#define EIO 4      // KSVM_ERR_IO: falha de E/S no host
#define EAGAIN 5   // KSVM_ERR_AGAIN: nada disponível agora
#define EKSEOF 6   // KSVM_ERR_EOF: fim da entrada (sem equivalente POSIX)
#define ENOMEM 12  // malloc sem memória
#define EDOM 33
#define ERANGE 34  // strtol/strtoul fora da faixa

#endif
