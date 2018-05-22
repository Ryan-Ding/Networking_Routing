all: ls_router manager_send

vec_router: main.c monitor_neighbors.c
	gcc -pthread -o vec_router main.c monitor_neighbors.c

ls_router: main.c monitor_neighbors.c ls_router.c
	gcc -pthread -o ls_router main.c monitor_neighbors.c ls_router.c

manager_send: manager_send.c
	gcc -o manager_send manager_send.c

.PHONY: clean
clean:
	rm ls_router manager_send