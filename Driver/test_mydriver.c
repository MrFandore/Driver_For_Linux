#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/mydriver"
#define BUFFER_SIZE 1024

// IOCTL команды
#define GET_STATS 0x01
#define CLEAR_STATS 0x02
#define GET_BUFFER_SIZE 0x03

void print_menu() {
    printf("\n=== Тест драйвера MYDRIVER ===\n");
    printf("1. Прочитать из устройства\n");
    printf("2. Записать в устройство\n");
    printf("3. Получить статистику (IOCTL)\n");
    printf("4. Очистить статистику (IOCTL)\n");
    printf("5. Получить размер буфера (IOCTL)\n");
    printf("6. Считать с разных позиций\n");
    printf("7. Выход\n");
    printf("Выбор: ");
}

int main() {
    int fd, choice;
    char buffer[BUFFER_SIZE];
    
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Не удалось открыть устройство");
        printf("Проверьте, загружен ли драйвер: lsmod | grep mydriver\n");
        return 1;
    }
    
    while(1) {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            printf("Ошибка ввода\n");
            while(getchar() != '\n');
            continue;
        }
        getchar(); // Убрать \n из буфера
        
        switch(choice) {
            case 1: // Чтение
                {
                    int bytes;
                    off_t offset = lseek(fd, 0, SEEK_CUR);
                    memset(buffer, 0, BUFFER_SIZE);
                    bytes = read(fd, buffer, BUFFER_SIZE - 1);
                    if (bytes < 0) {
                        perror("Ошибка чтения");
                    } else {
                        printf("Прочитано %d байт с позиции %ld: %s\n", bytes, (long)offset, buffer);
                    }
                    // Не сбрасываем позицию автоматически
                }
                break;
                
            case 2: // Запись
                {
                    off_t offset = lseek(fd, 0, SEEK_CUR);
                    printf("Введите строку для записи (текущая позиция: %ld): ", (long)offset);
                    fgets(buffer, BUFFER_SIZE - 1, stdin);
                    buffer[strcspn(buffer, "\n")] = 0;
                    
                    int bytes = write(fd, buffer, strlen(buffer));
                    if (bytes < 0) {
                        perror("Ошибка записи");
                    } else {
                        printf("Записано %d байт\n", bytes);
                    }
                }
                break;
                
            case 3: // Получить статистику
                {
                    int stats[3];
                    if (ioctl(fd, GET_STATS, stats) == 0) {
                        printf("Статистика драйвера:\n");
                        printf("  Чтений: %d\n", stats[0]);
                        printf("  Записей: %d\n", stats[1]);
                        printf("  Размер данных: %d байт\n", stats[2]);
                    } else {
                        perror("Ошибка IOCTL");
                    }
                }
                break;
                
            case 4: // Очистить статистику
                if (ioctl(fd, CLEAR_STATS, 0) == 0) {
                    printf("Статистика очищена\n");
                } else {
                    perror("Ошибка IOCTL");
                }
                break;
                
            case 5: // Получить размер буфера
                {
                    int size;
                    if (ioctl(fd, GET_BUFFER_SIZE, &size) == 0) {
                        printf("Размер буфера драйвера: %d байт\n", size);
                    } else {
                        perror("Ошибка IOCTL");
                    }
                }
                break;
                
            case 6: // Тест позиционирования
                {
                    printf("Тест чтения с разных позиций:\n");
                    for (int i = 0; i < 3; i++) {
                        memset(buffer, 0, BUFFER_SIZE);
                        lseek(fd, i * 5, SEEK_SET);
                        int bytes = read(fd, buffer, 10);
                        if (bytes < 0) {
                            perror("Ошибка чтения");
                        } else {
                            printf("Смещение %d: %d байт -> '%.10s'\n", 
                                   i * 5, bytes, buffer);
                        }
                    }
                    // Возвращаемся в начало
                    lseek(fd, 0, SEEK_SET);
                }
                break;
                
            case 7: // Выход
                close(fd);
                printf("Выход...\n");
                return 0;
                
            default:
                printf("Неверный выбор\n");
        }
    }
    
    close(fd);
    return 0;
}