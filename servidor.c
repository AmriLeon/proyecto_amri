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

// Prototipos de funciones
void guardar_kardex(const char* matricula, ResultadoAcademico* resultado_academico, ResultadoPsicometrico* resultado_psicometrico);

// Estructura para el kardex completo
typedef struct {
    char matricula[20];
    ResultadoAcademico resultados_academicos;
    ResultadoPsicometrico resultados_psicometricos;
} Kardex;

// Función para cargar preguntas desde archivo
#include <time.h>

int cargar_preguntas(const char* archivo, Pregunta* preguntas) {
    // Verificar si el archivo existe
    if (access(archivo, F_OK) != 0) {
        printf("Error: El archivo %s no existe\n", archivo);
        return 0;
    }

    // Verificar si el archivo es legible
    if (access(archivo, R_OK) != 0) {
        printf("Error: No se tienen permisos de lectura para el archivo %s\n", archivo);
        return 0;
    }

    FILE* f = fopen(archivo, "r");
    if (!f) {
        printf("Error al abrir el archivo: %s\n", archivo);
        return 0;
    }
    
    static int semilla_inicializada = 0;
    if (!semilla_inicializada) {
        srand(time(NULL));
        semilla_inicializada = 1;
    }
    
    Pregunta todas_preguntas[MAX_PREGUNTAS * 2];
    int total_preguntas = 0;
    char linea[512];
    
    while (total_preguntas < MAX_PREGUNTAS * 2) {
        // Leer la pregunta
        if (!fgets(linea, sizeof(linea), f)) break;
        
        // Eliminar el salto de línea y espacios en blanco
        size_t len = strlen(linea);
        while (len > 0 && (linea[len-1] == '\n' || linea[len-1] == '\r' || linea[len-1] == ' ')) {
            linea[--len] = '\0';
        }
        
        if (len <= 1) continue;
        
        // Verificar que la pregunta no esté vacía
        if (strlen(linea) == 0) continue;
        
        // Copiar la pregunta
        strncpy(todas_preguntas[total_preguntas].pregunta, linea, sizeof(todas_preguntas[total_preguntas].pregunta) - 1);
        todas_preguntas[total_preguntas].pregunta[sizeof(todas_preguntas[total_preguntas].pregunta) - 1] = '\0';
        
        // Leer y validar las tres opciones
        int opciones_leidas = 0;
        int opciones_validas = 1;
        
        for (int i = 0; i < 3; i++) {
            if (!fgets(linea, sizeof(linea), f)) {
                opciones_validas = 0;
                break;
            }
            
            // Eliminar el salto de línea y espacios en blanco
            len = strlen(linea);
            while (len > 0 && (linea[len-1] == '\n' || linea[len-1] == '\r' || linea[len-1] == ' ')) {
                linea[--len] = '\0';
            }
            
            // Validar formato de opción (A), B), C))
            if (len < 3 || linea[0] != 'A' + i || linea[1] != ')' || linea[2] != ' ') {
                opciones_validas = 0;
                break;
            }
            
            // Copiar la opción sin el prefijo
            strncpy(todas_preguntas[total_preguntas].opciones[i], linea + 3, sizeof(todas_preguntas[total_preguntas].opciones[i]) - 1);
            todas_preguntas[total_preguntas].opciones[i][sizeof(todas_preguntas[total_preguntas].opciones[i]) - 1] = '\0';
            opciones_leidas++;
        }
        
        // Leer y validar la respuesta
        if (opciones_validas && opciones_leidas == 3 && fgets(linea, sizeof(linea), f)) {
            // Eliminar el salto de línea y espacios en blanco
            len = strlen(linea);
            while (len > 0 && (linea[len-1] == '\n' || linea[len-1] == '\r' || linea[len-1] == ' ')) {
                linea[--len] = '\0';
            }
            
            // Validar formato de respuesta
            if (strncmp(linea, "RESPUESTA: ", 11) == 0) {
                char respuesta = toupper(linea[10]);
                if (respuesta >= 'A' && respuesta <= 'C') {
                    todas_preguntas[total_preguntas].respuesta = respuesta;
                    total_preguntas++;
                }
            }
        }
    }
    
    fclose(f);
    
    if (total_preguntas == 0) {
        printf("Error: No se encontraron preguntas válidas en el archivo %s. Verifique el formato del archivo\n", archivo);
        fclose(f);
        return 0;
    }

    // Verificar que el archivo no esté vacío
    if (ftell(f) == 0) {
        printf("Error: El archivo %s está vacío\n", archivo);
        fclose(f);
        return 0;
    }
    
    // Seleccionar aleatoriamente MAX_PREGUNTAS preguntas
    int num_preguntas_seleccionar = (total_preguntas < MAX_PREGUNTAS) ? total_preguntas : MAX_PREGUNTAS;
    int preguntas_seleccionadas = 0;
    int indices_usados[MAX_PREGUNTAS * 2] = {0};
    
    while (preguntas_seleccionadas < num_preguntas_seleccionar) {
        int indice_aleatorio = rand() % total_preguntas;
        
        if (!indices_usados[indice_aleatorio]) {
            preguntas[preguntas_seleccionadas] = todas_preguntas[indice_aleatorio];
            indices_usados[indice_aleatorio] = 1;
            preguntas_seleccionadas++;
        }
    }
    
    return num_preguntas_seleccionar;
}

// Función para manejar la conexión con el cliente
void enviar_examen_academico(int sock, const char* matricula) {
    Pregunta preguntas_mate[MAX_PREGUNTAS];
    Pregunta preguntas_espanol[MAX_PREGUNTAS];
    Pregunta preguntas_ingles[MAX_PREGUNTAS];
    
    int num_mate = cargar_preguntas("preguntas_mate.txt", preguntas_mate);
    int num_espanol = cargar_preguntas("preguntas_espanol.txt", preguntas_espanol);
    int num_ingles = cargar_preguntas("preguntas_ingles.txt", preguntas_ingles);
    
    ResultadoAcademico resultado = {0, 0, 0, 0.0};
    
    // Enviar número total de preguntas por materia
    send(sock, &num_mate, sizeof(int), 0);
    send(sock, &num_espanol, sizeof(int), 0);
    send(sock, &num_ingles, sizeof(int), 0);
    
    // Enviar y procesar preguntas de matemáticas
    for (int i = 0; i < num_mate; i++) {
        send(sock, &preguntas_mate[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        recv(sock, &respuesta_usuario, 1, 0);
        if (respuesta_usuario == preguntas_mate[i].respuesta) {
            resultado.matematicas++;
        }
    }
    
    // Enviar y procesar preguntas de español
    for (int i = 0; i < num_espanol; i++) {
        send(sock, &preguntas_espanol[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        recv(sock, &respuesta_usuario, 1, 0);
        if (respuesta_usuario == preguntas_espanol[i].respuesta) {
            resultado.espanol++;
        }
    }
    
    // Enviar y procesar preguntas de inglés
    for (int i = 0; i < num_ingles; i++) {
        send(sock, &preguntas_ingles[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        recv(sock, &respuesta_usuario, 1, 0);
        if (respuesta_usuario == preguntas_ingles[i].respuesta) {
            resultado.ingles++;
        }
    }
    
    // Calcular promedio
    resultado.promedio = (float)(resultado.matematicas + resultado.espanol + resultado.ingles) / 3.0;
    
    // Guardar resultado en el kardex
    ResultadoPsicometrico resultado_psico = {0}; // Resultado vacío para el kardex
    guardar_kardex(matricula, &resultado, &resultado_psico);
    
    // Enviar resultados al cliente
    send(sock, &resultado, sizeof(ResultadoAcademico), 0);
}

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

void enviar_kardex(int sock, const char* matricula) {
    FILE* f = fopen("kardex.txt", "r");
    if (f == NULL) {
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

void enviar_test_psicometrico(int sock) {
    Pregunta preguntas_visual[MAX_PREGUNTAS];
    Pregunta preguntas_razon[MAX_PREGUNTAS];
    int num_visual = cargar_preguntas("preguntas_visual.txt", preguntas_visual);
    int num_razon = cargar_preguntas("preguntas_razonamiento.txt", preguntas_razon);
    
    // Enviar número total de preguntas
    int total = num_visual + num_razon;
    send(sock, &total, sizeof(int), 0);
    
    int correctas = 0;
    
    // Enviar preguntas visuales
    for (int i = 0; i < num_visual; i++) {
        send(sock, &preguntas_visual[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        recv(sock, &respuesta_usuario, 1, 0);
        
        // Enviar si es correcta
        char es_correcta = (respuesta_usuario == preguntas_visual[i].respuesta);
        if (es_correcta) correctas++;
        send(sock, &es_correcta, 1, 0);
    }
    
    // Enviar preguntas de razonamiento
    for (int i = 0; i < num_razon; i++) {
        send(sock, &preguntas_razon[i], sizeof(Pregunta), 0);
        char respuesta_usuario;
        recv(sock, &respuesta_usuario, 1, 0);
        
        // Enviar si es correcta
        char es_correcta = (respuesta_usuario == preguntas_razon[i].respuesta);
        if (es_correcta) correctas++;
        send(sock, &es_correcta, 1, 0);
    }
    
    // Guardar resultados
    ResultadoPsicometrico resultado = {
        correctas,
        total,
        (float)correctas / total * 100,
        "" // La fecha se establecerá en el cliente
    };
    
    // Enviar resultado completo al cliente
    send(sock, &resultado, sizeof(ResultadoPsicometrico), 0);
}

void *manejar_cliente(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[MAX_BUFFER] = {0};
    int opcion;
    char matricula[20] = {0};
    
    while(1) {
        // Recibir opción del menú
        if (recv(sock, &opcion, sizeof(int), 0) <= 0) {
            break; // Si el cliente se desconecta
        }
        
        // Si el cliente elige salir
        if (opcion == 5) {
            break;
        }
        
        switch(opcion) {
            case 1: {
                Usuario usuario;
                recv(sock, &usuario, sizeof(Usuario), 0);
                
                // Guardar en archivo
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
                
                // Enviar confirmación
                char *mensaje = "Registro exitoso";
                send(sock, mensaje, strlen(mensaje), 0);
                break;
            }
            case 2: {
                // Recibir matrícula
                recv(sock, matricula, sizeof(matricula), 0);
                enviar_test_psicometrico(sock);
                break;
            }
            case 3: {
                // Recibir matrícula
                recv(sock, matricula, sizeof(matricula), 0);
                enviar_examen_academico(sock, matricula);
                break;
            }
            case 4: {
                // Recibir matrícula
                recv(sock, matricula, sizeof(matricula), 0);
                enviar_kardex(sock, matricula);
                break;
            }
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
    
    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error al crear socket");
        exit(EXIT_FAILURE);
    }
    
    // Configurar socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Vincular socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }
    
    // Escuchar conexiones
    if (listen(server_fd, 3) < 0) {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }
    
    printf("\033[1;34mServidor iniciado. Esperando conexiones...\033[0m\n");
    
    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Error en accept");
            exit(EXIT_FAILURE);
        }
        
        int *new_sock = malloc(sizeof(int));
        *new_sock = new_socket;
        
        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, manejar_cliente, (void*)new_sock) < 0) {
            perror("Error al crear thread");
            return 1;
        }
        pthread_detach(thread_id);
    }
    
    return 0;
}