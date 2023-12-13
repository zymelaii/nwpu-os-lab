#include <assert.h>
#include <mmu.h>
#include <type.h>

ssize_t
kern_wait(int *wstatus)
{
	// 相比于fork来说，wait的实现简单很多
	// 语义实现比较清晰，没有fork那么多难点要处理，所以这里并不会给大家太多引导
	// 需要大家自己思考wait怎么实现。

	// 在实现之前你必须得读一遍文档`man 2 wait`
	// 了解到wait大概要做什么
	panic("Unimplement! Read The F**king Manual");

	// 当然读文档是一方面，最重要的还是代码实现
	// wait系统调用与exit系统调用关系密切，所以在实现wait之前需要先读一遍exit为好
	// 可能读完exit的代码你可能知道wait该具体做什么了
	panic("Unimplement! Read The F**king Source Code");

	// 接下来就是你自己的实现了，我们在设计的时候这段代码不会有太大问题
	// 在实现完后你任然要对自己来个灵魂拷问
	// 1. 上锁上了吗？所有临界情况都考虑到了吗？（永远要相信有各种奇奇怪怪的并发问题）
	// 2. 所有错误情况都判断到了吗？错误情况怎么处理？（RTFM->`man 2 wait`）
	// 3. 是否所有的资源都正确回收了？
	// 4. 你写的代码真的符合wait语义吗？
	panic("Unimplement! soul torture");
	return 0;
}

ssize_t
do_wait(int *wstatus)
{
	assert((uintptr_t)wstatus < KERNBASE);
	assert((uintptr_t)wstatus + sizeof(wstatus) < KERNBASE);
	return kern_wait(wstatus);
}