#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>

struct birthday {
  int day;
  int month;
  int year;
  struct list_head list;
};

static LIST_HEAD(birthday_list);        // Head 포인터 선언하기

struct birthday *createBirthday(int day1, int month1, int year1) {
  // 본 함수는 구조체 birthday의 포인터를 반환함
  /* TODO: 생일을 위한 메모리를 할당하고, 인자들을 채워 생일을 완성하세요. */
  struct birthday *n;
  n = kmalloc(sizeof(struct birthday), GFP_KERNEL); // 메모리 할당

  n->day = day1;
  n->month = month1;
  n->year = year1;
  
  return n;
}

int simple_init(void) {
  printk("INSTALL MODULE: bdlist\n");
  /* TODO: 생일 목록을 하나씩 생성하는대로 연결리스트에 연결시키세요 (노드 삽입).*/
  struct birthday *node_pointer;
  
  // node_pointer는 새로운 struct birthday의 첫번째 요소에 대한 포인터를 가리키고 있음
  
  node_pointer = createBirthday(23,2,1995); 
  list_add_tail(&node_pointer->list, &birthday_list);      
  /* 
    연결 리스트인 birthday_list에 새로운 노드를 추가하여 작업을 수행함
    &node_pointer->list : 새 노드의 리스트 항목을 가리키는 포인터
    &birthday_list : 연결 리스트의 헤드 노드를 가리키는 포인터
  */
  node_pointer = createBirthday(19,4,1967);
  list_add_tail(&node_pointer->list, &birthday_list);
  node_pointer = createBirthday(7,2,1964);
  list_add_tail(&node_pointer->list, &birthday_list);
  
  /* TODO: 완성된 연결리스트를 탐색하는 커널 함수를 사용하여 출력하세요. */
  struct birthday *cursor;
  
  list_for_each_entry(cursor, &birthday_list, list) {
    printk("OS Module : Day %d, %d, %d \n", cursor->day, cursor->month, cursor->year);
  }
  /*
    birthday_list 리스트에 있는 모든 노드를 반복하며, 각 노드의 day, month, year 값을 출력함
    <매개변수>
    cursor : 연결 리스트의 각 노드에 대한 포인터 변수
    &birthday_list : 연결 리스트의 헤드 노드를 가리키는 포인터
    list : 연결 리스트에 사용되는 구조체 멤버 이름
  */
  return 0;
}

void simple_exit(void) {
  /* 모듈을 제거할 때는 생성한 연결 리스트도 하나씩 제거하며 끝내도록 하세요. */
  
  /* 제거를 하기 전에 리스트가 "비어있을 경우""에 대한 예외처리를 하는게 좋겠죠? */
  if(list_empty(&birthday_list)) {
    printk("List is Empty\n");
    return;
  }

  /* TODO: 이제 본격적으로 연결리스트를 탐색하면서 하나씩 제거하도록 하시면 됩니다. */

  struct birthday *cur_node;
  struct list_head *cursor, *next;
  list_for_each_safe(cursor, next, &birthday_list) {    // 안전하게 한 번에 하나의 노드를 삭제함
    cur_node = list_entry(cursor, struct birthday, list);   // struct birthday 주소를 반환함
    /* 
      cursor 포인터가 가리키는 노드가 birthday 구조체에 속해 있으며
      birthday 구조체의 리스트 멤버는 list임을 나타냄
      -> 현재 노드가 birthday 구조체에 속하는지 확인하고, cur_node 포인터 변수를 사용하여 현재 노드를 참조함
    */
    printk("OS Module : Removing %d, %d, %d \n", cur_node->day, cur_node->month, cur_node->year);
    list_del(cursor);   // cursor : 삭제할 구조체의 list_head 주소값 -> 리스트에서 제거
    kfree(cur_node);    // cur_node : 삭제할 구조체 메모리의 주소값 -> 노드에 할당한 메모리 해제    
    /* list_del()과 kfree() 함수를 이용하여 각 노드를 삭제함*/
  }

  printk("REMOVE MODULE: bdlist\n");
  return 0;

}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("make a list of birthdays and print");
MODULE_AUTHOR("이찬영_2019098068");