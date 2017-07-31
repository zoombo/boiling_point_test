/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: dimasik
 *
 * Created on 12 июля 2017 г., 16:25
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
// For sockets
#include <sys/socket.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <sys/epoll.h>
/*
 * 
 */

/*
 * 
 */
#define nl printf("\n")
#define uint unsigned int

#define PORT 4500
#define BUFF_LEN 10

/*
 * 
 */


int main(int argc, char** argv) {

    int status = 0;
    struct sockaddr_in serv_addr = {};
    int s32listener_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s32listener_fd == -1) {
        printf("Error create socket.\n");
        return -1;
    }

    int optval = 1;
    /* Когда мы закрываем сокет, он продолжает висеть некоторое время с 
     * состоянием TIME_WAIT и пока ОС его не закроет сокет с такими же 
     * параметрами (с таким же портом) открыть нельзя.
     * setsockopt можно задать параметр SO_REUSEADDR и тогда сокет можно
     * использовать повторно сразуже.
     */
    setsockopt(s32listener_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    // Повесить на все доступные интерфейсы.
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    status = bind(s32listener_fd, (struct sockaddr*) &serv_addr, sizeof (serv_addr));
    if (status == -1) {
        printf("Error bind socket.\n");
        return -1;
    }

    status = listen(s32listener_fd, 10);
    if (status == -1) {
        printf("Error listen socket.\n");
        return -1;
    }

    struct epoll_event in_ev = {}, out_ev = {};
    int mepoll_fd;

    mepoll_fd = epoll_create1(0);
    if (mepoll_fd == -1) {
        printf("Create epoll error.\n");
        return -1;
    }
    
    int s32accept_fd = 0;
    // char *msg = "Hello!";
    char *buff;
    
    while (true) {

        s32accept_fd = accept(s32listener_fd, 0, 0);

        in_ev.events = EPOLLIN;
        in_ev.data.fd = s32accept_fd;
        status = epoll_ctl(mepoll_fd, EPOLL_CTL_ADD, s32accept_fd, &in_ev);
        if (status == -1) {
            printf("epoll_ctl --> EPOLL_CTL_ADD error.\n");
            return -1;
        }

        buff = malloc(sizeof (char) * BUFF_LEN);
        int recv_bytes = 0, recv_full = 0;

        do {
            recv_bytes = recv(s32accept_fd, buff + recv_full, BUFF_LEN, 0);
            recv_full += recv_bytes;
            if (recv_bytes == BUFF_LEN) {
                buff = realloc(buff, recv_full + (sizeof (char) * BUFF_LEN));
            }
        } while (epoll_wait(mepoll_fd, &out_ev, 1, 0) != 0);
        // На всяк случай, чтобы потом проблем с переполненными буферами небыло.
        *(buff + recv_full) = '\0';

        // DEBUG.
        printf("%s", buff);
        printf("_test\n");
        // END_DEBUG.
        //exit(0);

        if (!strcmp(buff, "shutdown")) {
            close(s32accept_fd);
            free(buff);
            exit(0);
        }

        // Обойдемся пока без регулярок и PCRE.            
        _Bool found_S = false;
        int finding_str_len = 0;
        for (unsigned int i = 0; i < recv_full; i++) {
            if (buff[i] == 'S' && found_S == false) {
                found_S = true;
                continue;
            }
            if (found_S && buff[i] != 'E') {
                finding_str_len++;
            }
            if (buff[i] == 'E')
                break;
        }

        printf("\nWTF???!\n");

        free(buff);

        char send_buff[sizeof (int)] = {};
        sprintf(send_buff, "%d", finding_str_len);
        // sprintf(send_buff, "%d", 10);
        send(s32accept_fd, send_buff, sizeof (int), 0);
        // DEBUG.
        //send(s32accept_fd, "\n", sizeof ("\n"), 0);
        // END_DEBUG.
        close(s32accept_fd);

    }

    shutdown(s32listener_fd, SHUT_RDWR);
    close(s32listener_fd);

    return (EXIT_SUCCESS);
}

