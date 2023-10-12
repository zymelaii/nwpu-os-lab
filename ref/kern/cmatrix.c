#include "type.h"
#include "cmatrix.h"
#include "terminal.h"

static size_t random_seed = 114;
static size_t 
rand() {
	return ( ((random_seed = random_seed * 214013L
		+ 2531011L) >> 16) & 0x7fff );
}

struct cmatrix_node {
	u8			row;
	u8			character;
	struct cmatrix_node 	*pre;
	struct cmatrix_node 	*nxt;
};

struct cmatrix_info {
	u8			column;
	u8			max_length;
	u8			now_length;
	struct cmatrix_node	*head;
	struct cmatrix_node	*tail;
};

typedef struct cmatrix_node cm_node;
typedef struct cmatrix_info cm_info;

#define NODE_POOL_SIZE ((TERMINAL_ROW) * (TERMINAL_COLUMN))
static cm_node node_pool[NODE_POOL_SIZE];

static cm_node* 
alloc_node()
{
	for (int i = 0; i < NODE_POOL_SIZE; i++)
		if (node_pool[i].character == 0)
			return &node_pool[i];
	return NULL;
}

static void 
free_node(cm_node *node_ptr)
{
	node_ptr->row = 0;
	node_ptr->character = 0;
	if (node_ptr->pre != NULL) {
		node_ptr->pre->nxt = NULL;
		node_ptr->pre = NULL;
	}
	if (node_ptr->nxt != NULL) {
		node_ptr->nxt->pre = NULL;
		node_ptr->nxt = NULL;
	}
}

#define INFO_POOL_SIZE (TERMINAL_COLUMN)
static cm_info info_pool[INFO_POOL_SIZE];

const u8 char_pool[] = {
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z',
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	'!','@','#','$','^','&','*','(',')','{','}','[',']',
	'|','?','/',':',';','`','~','>','<',',','.',
};

static void 
init_info(cm_info *info_ptr)
{
	info_ptr->max_length = rand() % (TERMINAL_ROW - 1) + 1;
	info_ptr->head = info_ptr->tail = NULL;
}

static void
expand_info(cm_info *info_ptr)
{
	u8 character = char_pool[rand() % ARRAY_SIZE(char_pool)];

	if (info_ptr->head == NULL) {
		cm_node node = {
			.row = 0,
			.character = character,
			.pre = NULL,
			.nxt = NULL,
		};
		cm_node *node_ptr = alloc_node();
		*node_ptr = node;
		// 修改info
		info_ptr->head = node_ptr;
		info_ptr->tail = node_ptr;
		info_ptr->now_length++;
		// 打印字符
#ifdef TESTS
		struct color_char black_green = {
			.background = BLACK,
			.foreground = GREEN,
			.print_char = info_ptr->head->character,
			.print_pos = TERMINAL_POS(info_ptr->head->row, 
							info_ptr->column)
		};
		kprintf(0, "%s", black_green);
#else
		kprintf(TERMINAL_POS(info_ptr->head->row, 
							info_ptr->column), 
			"%b%c", GREEN, info_ptr->head->character);
#endif

	} else if (info_ptr->head->row < TERMINAL_ROW - 1) {
		cm_node node = {
			.row = info_ptr->head->row + 1,
			.character = character,
			.pre = NULL,
			.nxt = info_ptr->head,
		};
		cm_node *node_ptr = alloc_node();
		*node_ptr = node;
		// 打印字符
#ifdef TESTS
		struct color_char black_green = {
			.background = BLACK,
			.foreground = GREEN,
			.print_char = info_ptr->head->character,
			.print_pos = TERMINAL_POS(info_ptr->head->row, 
							info_ptr->column)
		};
		kprintf(0, "%s", black_green);
#else
		kprintf(TERMINAL_POS(info_ptr->head->row, 
							info_ptr->column), 
			"%f%c", GREEN, info_ptr->head->character);
#endif
		// 修改info
		info_ptr->head->pre = node_ptr;
		info_ptr->head = node_ptr;
		info_ptr->now_length++;
		// 打印字符
#ifdef TESTS
		struct color_char green_white = {
			.background = GREEN,
			.foreground = WHITE,
			.print_char = info_ptr->head->character,
			.print_pos = TERMINAL_POS(info_ptr->head->row, 
							info_ptr->column)
		};
		kprintf(0, "%s", green_white);
#else
		kprintf(TERMINAL_POS(info_ptr->head->row, 
							info_ptr->column), 
			"%b%c", GREEN, info_ptr->head->character);
#endif
	} else {
		// 打印字符
#ifdef TESTS
		struct color_char black_green = {
			.background = BLACK,
			.foreground = GREEN,
			.print_char = info_ptr->head->character,
			.print_pos = TERMINAL_POS(info_ptr->head->row, 
							info_ptr->column)
		};
		kprintf(0, "%s", black_green);
#else
		kprintf(TERMINAL_POS(info_ptr->head->row, 
							info_ptr->column), 
			"%f%c", GREEN, info_ptr->head->character);
#endif
	}

	if (info_ptr->now_length > info_ptr->max_length
	|| info_ptr->head->row == TERMINAL_ROW - 1) {
		cm_node *node_ptr = info_ptr->tail;
		// 打印字符
		kprintf(TERMINAL_POS(info_ptr->tail->row, info_ptr->column), 
			"%c", ' ');
		// 修改info
		info_ptr->tail = info_ptr->tail->pre;
		info_ptr->now_length--;
		// 释放内存
		free_node(node_ptr);
	}
}

void 
cmatrix_start()
{
	clear_screen();
	for (int i = 0; i < TERMINAL_COLUMN; i++)
		info_pool[i].column = i;
	while(1) {
		for (int i = 0; i < TERMINAL_COLUMN; i++) {
			cm_info *info_ptr = &info_pool[i];
			if (info_ptr->now_length == 0)
				init_info(info_ptr);
			expand_info(info_ptr);
		}
		for (int i = 0; i < 1e7; i++) {
			// do nothing
		}
	}
}