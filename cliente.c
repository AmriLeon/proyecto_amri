#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 8080
#define MAX_BUFFER 1024
#define SERVER_IP "127.0.0.1"

// Estructura para almacenar datos del usuario
typedef struct {
    char nombre[50];
    char matricula[20];
    char carrera[50];
    int edad;
    char genero;
    int semestre;
    char password[50];
} Usuario;

typedef struct {
    char pregunta[256];
    char opciones[3][64];
    char respuesta;
} Pregunta;

typedef struct {
    int matematicas;
    int espanol;
    int ingles;
    float promedio;
} ResultadoAcademico;

typedef struct {
    int correctas;
    int total;
    float porcentaje;
    char fecha[20];
} ResultadoPsicometrico;

typedef struct {
    char matricula[20];
    ResultadoAcademico resultados_academicos;
    ResultadoPsicometrico resultados_psicometricos;
} Kardex;

// Función para mostrar el kardex
void mostrar_kardex(int sock) {
    char matricula[20];
    printf("\033[1;32m=== VER KARDEX ===\033[0m\n\n");
    printf("Ingrese su matrícula: ");
    scanf(" %[^\n]", matricula);
    
    // Enviar matrícula al servidor
    send(sock, matricula, sizeof(matricula), 0);
    
    // Recibir respuesta
    char buffer[MAX_BUFFER];
    Kardex kardex;
    int bytes_recibidos = recv(sock, buffer, sizeof(buffer), MSG_PEEK);
    
    if (bytes_recibidos == sizeof(Kardex)) {
        // Si recibimos un Kardex completo
        recv(sock, &kardex, sizeof(Kardex), 0);
        
        printf("\n\033[1;34m=== RESULTADOS ACADÉMICOS ===\033[0m\n");
        printf("Matemáticas: %d\n", kardex.resultados_academicos.matematicas);
        printf("Español: %d\n", kardex.resultados_academicos.espanol);
        printf("Inglés: %d\n", kardex.resultados_academicos.ingles);
        printf("Promedio general: %.2f\n", kardex.resultados_academicos.promedio);
        
        printf("\n\033[1;34m=== RESULTADOS TEST PSICOMÉTRICO ===\033[0m\n");
        printf("Respuestas correctas: %d de %d\n", kardex.resultados_psicometricos.correctas, kardex.resultados_psicometricos.total);
        printf("Porcentaje de aciertos: %.2f%%\n", kardex.resultados_psicometricos.porcentaje);
        printf("Fecha: %s\n", kardex.resultados_psicometricos.fecha);
    } else {
        // Si recibimos un mensaje de error
        recv(sock, buffer, MAX_BUFFER, 0);
        printf("\n\033[1;31m%s\033[0m\n", buffer);
    }
    
    printf("\nPresiona Enter para continuar...");
    getchar();
    getchar();
}

// Función para limpiar la pantalla
void limpiar_pantalla() {
    system("clear");
}

// Función para mostrar el menú principal
void mostrar_menu() {
    printf("\033[1;34m╔══════════════════════════╗\n");
    printf("║    SISTEMA DE EXAMEN    ║\n");
    printf("╚══════════════════════════╝\033[0m\n\n");
    printf("1. Registrarse\n");
    printf("2. Iniciar Test Psicométrico\n");
    printf("3. Realizar Examen Académico\n");
    printf("4. Ver Cardex\n");
    printf("5. Salir\n\n");
    printf("Seleccione una opción: ");
}

void realizar_examen_academico(int sock) {
    int num_mate, num_espanol, num_ingles;
    recv(sock, &num_mate, sizeof(int), 0);
    recv(sock, &num_espanol, sizeof(int), 0);
    recv(sock, &num_ingles, sizeof(int), 0);
    
    Pregunta pregunta;
    ResultadoAcademico resultado;
    
    printf("\033[1;32m=== EXAMEN DE MATEMÁTICAS ===\033[0m\n\n");
    for (int i = 0; i < num_mate; i++) {
        recv(sock, &pregunta, sizeof(Pregunta), 0);
        
        printf("\nPregunta %d:\n%s\n", i + 1, pregunta.pregunta);
        printf("A) %s\n", pregunta.opciones[0]);
        printf("B) %s\n", pregunta.opciones[1]);
        printf("C) %s\n", pregunta.opciones[2]);
        
        char respuesta;
        do {
            printf("Tu respuesta (A/B/C): ");
            scanf(" %c", &respuesta);
            respuesta = toupper(respuesta);
        } while (respuesta != 'A' && respuesta != 'B' && respuesta != 'C');
        
        send(sock, &respuesta, 1, 0);
    }
    
    printf("\n\033[1;32m=== EXAMEN DE ESPAÑOL ===\033[0m\n\n");
    for (int i = 0; i < num_espanol; i++) {
        recv(sock, &pregunta, sizeof(Pregunta), 0);
        
        printf("\nPregunta %d:\n%s\n", i + 1, pregunta.pregunta);
        printf("A) %s\n", pregunta.opciones[0]);
        printf("B) %s\n", pregunta.opciones[1]);
        printf("C) %s\n", pregunta.opciones[2]);
        
        char respuesta;
        do {
            printf("Tu respuesta (A/B/C): ");
            scanf(" %c", &respuesta);
            respuesta = toupper(respuesta);
        } while (respuesta != 'A' && respuesta != 'B' && respuesta != 'C');
        
        send(sock, &respuesta, 1, 0);
    }
    
    printf("\n\033[1;32m=== EXAMEN DE INGLÉS ===\033[0m\n\n");
    for (int i = 0; i < num_ingles; i++) {
        recv(sock, &pregunta, sizeof(Pregunta), 0);
        
        printf("\nPregunta %d:\n%s\n", i + 1, pregunta.pregunta);
        printf("A) %s\n", pregunta.opciones[0]);
        printf("B) %s\n", pregunta.opciones[1]);
        printf("C) %s\n", pregunta.opciones[2]);
        
        char respuesta;
        do {
            printf("Tu respuesta (A/B/C): ");
            scanf(" %c", &respuesta);
            respuesta = toupper(respuesta);
        } while (respuesta != 'A' && respuesta != 'B' && respuesta != 'C');
        
        send(sock, &respuesta, 1, 0);
    }
    
    // Recibir resultados
    recv(sock, &resultado, sizeof(ResultadoAcademico), 0);
    
    printf("\n\033[1;34m=== RESULTADOS DEL EXAMEN ACADÉMICO ===\033[0m\n");
    printf("Matemáticas: %d/%d\n", resultado.matematicas, num_mate);
    printf("Español: %d/%d\n", resultado.espanol, num_espanol);
    printf("Inglés: %d/%d\n", resultado.ingles, num_ingles);
    printf("Promedio general: %.2f\n", resultado.promedio);
    
    if (resultado.promedio >= 8.0) {
        printf("\033[1;32m¡Excelente desempeño!\033[0m\n");
    } else if (resultado.promedio >= 6.0) {
        printf("\033[1;33mBuen trabajo, pero hay espacio para mejorar.\033[0m\n");
    } else {
        printf("\033[1;31mNecesitas estudiar más.\033[0m\n");
    }
    
    printf("\nPresiona Enter para continuar...");
    getchar();
    getchar();
}

void realizar_test_psicometrico(int sock) {
    int total_preguntas;
    recv(sock, &total_preguntas, sizeof(int), 0);
    
    printf("\033[1;32m=== TEST PSICOMÉTRICO ===\033[0m\n\n");
    printf("Total de preguntas: %d\n\n", total_preguntas);
    
    int correctas = 0;
    Pregunta pregunta;
    
    for (int i = 0; i < total_preguntas; i++) {
        recv(sock, &pregunta, sizeof(Pregunta), 0);
        
        printf("\nPregunta %d:\n%s\n", i + 1, pregunta.pregunta);
        printf("A) %s\n", pregunta.opciones[0]);
        printf("B) %s\n", pregunta.opciones[1]);
        printf("C) %s\n", pregunta.opciones[2]);
        
        char respuesta;
        do {
            printf("Tu respuesta (A/B/C): ");
            scanf(" %c", &respuesta);
            respuesta = toupper(respuesta);
        } while (respuesta != 'A' && respuesta != 'B' && respuesta != 'C');
        
        send(sock, &respuesta, 1, 0);
        
        char es_correcta;
        recv(sock, &es_correcta, 1, 0);
        
        if (es_correcta) {
            printf("\033[1;32m¡Correcto!\033[0m\n");
            correctas++;
        } else {
            printf("\033[1;31mIncorrecto\033[0m\n");
        }
    }
    
    float porcentaje = (float)correctas / total_preguntas * 100;
    printf("\n=== Resultados ===\n");
    printf("Respuestas correctas: %d de %d\n", correctas, total_preguntas);
    printf("Porcentaje de aciertos: %.2f%%\n", porcentaje);
    
    if (porcentaje >= 80) {
        printf("\033[1;32m¡Excelente desempeño!\033[0m\n");
    } else if (porcentaje >= 60) {
        printf("\033[1;33mBuen trabajo, pero hay espacio para mejorar.\033[0m\n");
    } else {
        printf("\033[1;31mNecesitas practicar más.\033[0m\n");
    }
    
    printf("\nPresiona Enter para continuar...");
    getchar();
    getchar();
}

// Función para registrar usuario
Usuario registrar_usuario() {
    Usuario usuario;
    limpiar_pantalla();
    printf("\033[1;32m=== REGISTRO DE USUARIO ===\033[0m\n\n");
    
    printf("Nombre: ");
    scanf(" %[^\n]", usuario.nombre);
    
    printf("Matrícula: ");
    scanf(" %[^\n]", usuario.matricula);
    
    printf("Carrera: ");
    scanf(" %[^\n]", usuario.carrera);
    
    printf("Edad: ");
    scanf("%d", &usuario.edad);
    
    printf("Género (M/F): ");
    scanf(" %c", &usuario.genero);
    
    printf("Semestre: ");
    scanf("%d", &usuario.semestre);
    
    printf("Contraseña: ");
    scanf(" %[^\n]", usuario.password);
    
    return usuario;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[MAX_BUFFER] = {0};
    int opcion;
    
    // Crear socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nError al crear el socket\n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convertir dirección IP
    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nDirección inválida o no soportada\n");
        return -1;
    }
    
    // Conectar al servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nError de conexión\n");
        return -1;
    }
    
    while(1) {
        limpiar_pantalla();
        mostrar_menu();
        scanf("%d", &opcion);
        
        switch(opcion) {
            case 1: {
                Usuario usuario = registrar_usuario();
                send(sock, &opcion, sizeof(int), 0);
                send(sock, &usuario, sizeof(Usuario), 0);
                recv(sock, buffer, MAX_BUFFER, 0);
                printf("\n%s\n", buffer);
                printf("\nPresione Enter para continuar...");
                getchar();
                getchar();
                break;
            }
            case 2: {
                send(sock, &opcion, sizeof(int), 0);
                // Solicitar matrícula antes del test
                char matricula[20];
                printf("\nIngrese su matrícula: ");
                scanf(" %[^\n]", matricula);
                send(sock, matricula, sizeof(matricula), 0);
                realizar_test_psicometrico(sock);
                break;
            }
            case 3: {
                send(sock, &opcion, sizeof(int), 0);
                // Solicitar matrícula antes del examen
                char matricula[20];
                printf("\nIngrese su matrícula: ");
                scanf(" %[^\n]", matricula);
                send(sock, matricula, sizeof(matricula), 0);
                realizar_examen_academico(sock);
                break;
            }
            case 4: {
                send(sock, &opcion, sizeof(int), 0);
                mostrar_kardex(sock);
                break;
            }
            case 5:
                close(sock);
                return 0;
            default:
                printf("\nOpción no válida\n");
                sleep(1);
        }
    }
    
    return 0;
}