#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_BUFFER 1024
#define MAX_PREGUNTAS 10

// Estructura para resultados del examen académico
typedef struct {
    int matematicas;
    int espanol;
    int ingles;
    float promedio;
} ResultadoAcademico;

// Estructura para resultados del test psicométrico
typedef struct {
    int correctas;
    int total;
    float porcentaje;
    char fecha[20];
} ResultadoPsicometrico;

// Prototipos de funciones
void guardar_kardex(const char* matricula, ResultadoAcademico* resultado_academico, ResultadoPsicometrico* resultado_psicometrico);

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

// Estructura para preguntas
typedef struct {
    char pregunta[256];
    char opciones[3][64];
    char respuesta;
} Pregunta;

// Estructura para el kardex completo
typedef struct {
    char matricula[20];
    ResultadoAcademico resultados_academicos;
    ResultadoPsicometrico resultados_psicometricos;
} Kardex;

// Función para cargar preguntas desde archivo
int cargar_preguntas(const char* archivo, Pregunta* preguntas) {
    FILE* f = fopen(archivo, "r");
    if (!f) return 0;
    
    int count = 0;
    char linea[512];
    
    while (count < MAX_PREGUNTAS && fgets(linea, sizeof(linea), f)) {
        if (strlen(linea) <= 1) continue;
        strcpy(preguntas[count].pregunta, linea);
        
        for (int i = 0; i < 3; i++) {
            if (!fgets(linea, sizeof(linea), f)) break;
            if (strlen(linea) > 2) {
                strcpy(preguntas[count].opciones[i], linea + 3);
                preguntas[count].opciones[i][strlen(preguntas[count].opciones[i]) - 1] = '\0';
            }
        }
        
        if (fgets(linea, sizeof(linea), f)) {
            if (strncmp(linea, "RESPUESTA:", 10) == 0) {
                preguntas[count].respuesta = linea[10];
                count++;
            }
        }
    }
    
    fclose(f);
    return count;
}

// Función para manejar el examen académico
void enviar_examen_academico(int sock, const char* matricula) {
    Pregunta preguntas_mate[MAX_PREGUNTAS];
    Pregunta preguntas_espanol[MAX_PREGUNTAS];
    Pregunta preguntas_ingles[MAX_PREGUNTAS];
    
    int num_mate = cargar_preguntas("preguntas_mate.txt", preguntas_mate);
    int num_espanol = cargar_preguntas("preguntas_espanol.txt", preguntas_espanol);
    int num_ingles = cargar_preguntas("preguntas_ingles.txt", preguntas_ingles);
    
    if (num_mate == 0 || num_espanol == 0 || num_ingles == 0) {
        char* mensaje = "Error: No se pudieron cargar las preguntas del examen";
        send(sock, mensaje, strlen(mensaje), 0);
        return;
    }
    
    ResultadoAcademico resultado = {0, 0, 0, 0.0};
    
    // Enviar número de preguntas
    send(sock, &num_mate, sizeof(int), 0);
    send(sock, &num_espanol, sizeof(int), 0);
    send(sock, &num_ingles, sizeof(int), 0);
    
    for (int i = 0; i < num_mate; i++) {
        send(sock, &preguntas_mate[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        recv(sock, &respuesta_usuario, 1, 0);
        if (respuesta_usuario == preguntas_mate[i].respuesta) {
            resultado.matematicas++;
        }
    }
    
    for (int i = 0; i < num_espanol; i++) {
        send(sock, &preguntas_espanol[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        recv(sock, &respuesta_usuario, 1, 0);
        if (respuesta_usuario == preguntas_espanol[i].respuesta) {
            resultado.espanol++;
        }
    }
    
    for (int i = 0; i < num_ingles; i++) {
        send(sock, &preguntas_ingles[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        recv(sock, &respuesta_usuario, 1, 0);
        if (respuesta_usuario == preguntas_ingles[i].respuesta) {
            resultado.ingles++;
        }
    }
    
    resultado.promedio = (float)(resultado.matematicas + resultado.espanol + resultado.ingles) / 3.0;

    ResultadoPsicometrico resultado_psico = {0}; // vacío temporal
    guardar_kardex(matricula, &resultado, &resultado_psico);

    send(sock, &resultado, sizeof(ResultadoAcademico), 0);
}

// Función para guardar resultados en archivo
void guardar_kardex(const char* matricula, ResultadoAcademico* resultado_academico, ResultadoPsicometrico* resultado_psicometrico) {
    FILE* f = fopen("kardex.txt", "a");
    if (f != NULL) {
        fprintf(f, "%s,%d,%d,%d,%.2f,%d,%d,%.2f,%s\n",
                matricula,
                resultado_academico->matematicas,
                resultado_academico->espanol,
                resultado_academico->ingles,
                resultado_academico->promedio,
                resultado_psicometrico->correctas,
                resultado_psicometrico->total,
                resultado_psicometrico->porcentaje,
                resultado_psicometrico->fecha);
        fclose(f);
    }
}

// Función para enviar kardex por matrícula
void enviar_kardex(int sock, const char* matricula) {
    FILE* f = fopen("kardex.txt", "r");
    if (!f) {
        char* mensaje = "No hay registros disponibles";
        send(sock, mensaje, strlen(mensaje), 0);
        return;
    }

    char linea[512];
    char mat_temp[20];
    Kardex kardex;
    int encontrado = 0;

    while (fgets(linea, sizeof(linea), f)) {
        sscanf(linea, "%[^,],%d,%d,%d,%f,%d,%d,%f,%s",
               mat_temp,
               &kardex.resultados_academicos.matematicas,
               &kardex.resultados_academicos.espanol,
               &kardex.resultados_academicos.ingles,
               &kardex.resultados_academicos.promedio,
               &kardex.resultados_psicometricos.correctas,
               &kardex.resultados_psicometricos.total,
               &kardex.resultados_psicometricos.porcentaje,
               kardex.resultados_psicometricos.fecha);

        if (strcmp(mat_temp, matricula) == 0) {
            encontrado = 1;
            strcpy(kardex.matricula, matricula);
            send(sock, &kardex, sizeof(Kardex), 0);
            break;
        }
    }

    if (!encontrado) {
        char* mensaje = "No se encontraron registros para esta matrícula";
        send(sock, mensaje, strlen(mensaje), 0);
    }

    fclose(f);
}

// Función para test psicométrico
void enviar_test_psicometrico(int sock) {
    Pregunta preguntas_visual[MAX_PREGUNTAS];
    Pregunta preguntas_razon[MAX_PREGUNTAS];
    int num_visual = cargar_preguntas("preguntas_visual.txt", preguntas_visual);
    int num_razon = cargar_preguntas("preguntas_razonamiento.txt", preguntas_razon);
    
    int total = num_visual + num_razon;
    send(sock, &total, sizeof(int), 0);
    
    int correctas = 0;
    
    for (int i = 0; i < num_visual; i++) {
        send(sock, &preguntas_visual[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        recv(sock, &respuesta_usuario, 1, 0);
        char es_correcta = (respuesta_usuario == preguntas_visual[i].respuesta);
        if (es_correcta) correctas++;
        send(sock, &es_correcta, 1, 0);
    }
    
    for (int i = 0; i < num_razon; i++) {
        send(sock, &preguntas_razon[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        recv(sock, &respuesta_usuario, 1, 0);
        char es_correcta = (respuesta_usuario == preguntas_razon[i].respuesta);
        if (es_correcta) correctas++;
        send(sock, &es_correcta, 1, 0);
    }

    ResultadoPsicometrico resultado = {
        correctas,
        total,
        (float)correctas / total * 100,
        "" // El cliente debe llenar la fecha
    };

    send(sock, &resultado, sizeof(ResultadoPsicometrico), 0);
}

// Hilo de cliente
void *manejar_cliente(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[MAX_BUFFER] = {0};
    int opcion;
    char matricula[20] = {0};
    
    recv(sock, &opcion, sizeof(int), 0);
    
    switch(opcion) {
        case 1: {
            Usuario usuario;
            recv(sock, &usuario, sizeof(Usuario), 0);
            
            FILE *f = fopen("registros.txt", "a");
            if (f != NULL) {
                fprintf(f, "%s,%s,%s,%d,%c,%d,%s\n", 
                        usuario.nombre, 
                        usuario.matricula,
                        usuario.carrera,
                        usuario.edad,
                        usuario.genero,
                        usuario.semestre,
                        usuario.password);
                fclose(f);
            }
            
            char *mensaje = "Registro exitoso";
            send(sock, mensaje, strlen(mensaje), 0);
            break;
        }
        case 2: {
            recv(sock, matricula, sizeof(matricula), 0);
            enviar_test_psicometrico(sock);
            break;
        }
        case 3: {
            recv(sock, matricula, sizeof(matricula), 0);
            enviar_examen_academico(sock, matricula);
            break;
        }
        case 4: {
            recv(sock, matricula, sizeof(matricula), 0);
            enviar_kardex(sock, matricula);
            break;
        }
    }
    
    free(socket_desc);
    close(sock);
    return 0;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error al crear socket");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 3) < 0) {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }
    
    printf("\033[1;34mServidor iniciado. Esperando conexiones...\033[0m\n");
    
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Error en accept");
            exit(EXIT_FAILURE);
        }
        
        int *new_sock = malloc(sizeof(int));
        *new_sock = new_socket;
        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, manejar_cliente, (void*)new_sock) < 0) {
            perror("Error al crear thread");
            return 1;
        }
        pthread_detach(thread_id);
    }

    return 0;
}
