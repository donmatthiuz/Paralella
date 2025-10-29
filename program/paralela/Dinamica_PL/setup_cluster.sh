#!/bin/bash
# setup_cluster.sh - Script para configurar y ejecutar el cluster MPI

echo "╔════════════════════════════════════════════════════════╗"
echo "║       CONFIGURACIÓN DE CLUSTER MPI PARA DES            ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# ==================== PASO 1: COMPILACIÓN ====================
compile() {
    echo "📦 Compilando programa..."
    mpicc -o des_bruteforce_dynamic des_bruteforce_dynamic.c -lcrypt -O3
    if [ $? -eq 0 ]; then
        echo "✅ Compilación exitosa"
    else
        echo "❌ Error en la compilación"
        exit 1
    fi
}

# ==================== PASO 2: CREAR HOSTFILE ====================
create_hostfile() {
    echo ""
    echo "📝 Creando archivo de hosts..."
    
    cat > hosts.txt << 'EOF'
# Archivo de configuración de hosts para MPI
# Formato: hostname slots=N
# 
# Ejemplo de configuración:
# localhost slots=4          # 4 procesos en máquina local
# 192.168.1.10 slots=4      # 4 procesos en nodo 1
# 192.168.1.11 slots=4      # 4 procesos en nodo 2
# node1.local slots=2       # 2 procesos en node1
#
# Configura tus nodos aquí:

localhost slots=4
EOF
    
    echo "✅ Archivo hosts.txt creado (edítalo con tus nodos)"
}

# ==================== PASO 3: VERIFICAR CONFIGURACIÓN SSH ====================
check_ssh() {
    echo ""
    echo "🔐 Verificando configuración SSH..."
    echo ""
    echo "Para un cluster MPI, necesitas:"
    echo "  1. SSH sin contraseña entre nodos"
    echo "  2. Mismo usuario en todos los nodos"
    echo "  3. MPI instalado en todos los nodos"
    echo "  4. Programa compilado en la misma ruta en todos los nodos"
    echo ""
    echo "Configurar SSH sin contraseña:"
    echo "  ssh-keygen -t rsa"
    echo "  ssh-copy-id usuario@nodo_remoto"
    echo ""
}

# ==================== PASO 4: TEST DE CONEXIÓN ====================
test_cluster() {
    echo ""
    echo "🧪 Probando conectividad del cluster..."
    
    if [ ! -f hosts.txt ]; then
        echo "❌ No existe hosts.txt"
        return 1
    fi
    
    echo ""
    mpirun --hostfile hosts.txt hostname
    
    if [ $? -eq 0 ]; then
        echo "✅ Cluster respondiendo correctamente"
    else
        echo "❌ Problemas de conectividad"
    fi
}

# ==================== PASO 5: DISTRIBUIR PROGRAMA ====================
distribute() {
    echo ""
    echo "📤 Distribuyendo programa a nodos..."
    
    if [ ! -f hosts.txt ]; then
        echo "❌ No existe hosts.txt"
        return 1
    fi
    
    # Leer hosts y copiar archivo
    while read line; do
        # Ignorar comentarios y líneas vacías
        [[ "$line" =~ ^#.*$ ]] && continue
        [[ -z "$line" ]] && continue
        
        host=$(echo $line | awk '{print $1}')
        
        if [ "$host" != "localhost" ]; then
            echo "  → Copiando a $host..."
            scp des_bruteforce_dynamic $host:$(pwd)/
            
            if [ $? -eq 0 ]; then
                echo "    ✅ Copiado a $host"
            else
                echo "    ❌ Error copiando a $host"
            fi
        fi
    done < hosts.txt
}

# ==================== PASO 6: EJECUTAR ====================
run() {
    if [ $# -lt 2 ]; then
        echo ""
        echo "❌ Uso: $0 run <archivo_cifrado> <texto_buscar> [num_procesos]"
        echo ""
        echo "Ejemplos:"
        echo "  $0 run texto.txt.enc \"es una prueba\" 8"
        echo "  $0 run archivo.enc \"password\" 16"
        return 1
    fi
    
    cipher_file=$1
    search_text=$2
    num_procs=${3:-8}
    
    if [ ! -f "$cipher_file" ]; then
        echo "❌ Archivo no encontrado: $cipher_file"
        return 1
    fi
    
    echo ""
    echo "🚀 Ejecutando bruteforce con $num_procs procesos..."
    echo "   Archivo: $cipher_file"
    echo "   Buscar: \"$search_text\""
    echo ""
    
    mpirun -np $num_procs \
           --hostfile hosts.txt \
           --mca btl_tcp_if_include eth0 \
           --mca oob_tcp_if_include eth0 \
           ./des_bruteforce_dynamic "$cipher_file" "$search_text"
}

# ==================== MENÚ PRINCIPAL ====================
show_menu() {
    echo ""
    echo "Opciones disponibles:"
    echo "  compile       - Compilar el programa"
    echo "  hostfile      - Crear archivo hosts.txt"
    echo "  check-ssh     - Ver info de configuración SSH"
    echo "  test          - Probar conectividad del cluster"
    echo "  distribute    - Copiar programa a todos los nodos"
    echo "  run           - Ejecutar bruteforce"
    echo "  full-setup    - Hacer configuración completa"
    echo ""
    echo "Ejemplo de uso completo:"
    echo "  ./setup_cluster.sh compile"
    echo "  ./setup_cluster.sh hostfile"
    echo "  # (editar hosts.txt con tus nodos)"
    echo "  ./setup_cluster.sh test"
    echo "  ./setup_cluster.sh distribute"
    echo "  ./setup_cluster.sh run texto.txt.enc \"prueba\" 8"
}

# ==================== CONFIGURACIÓN COMPLETA ====================
full_setup() {
    echo ""
    echo "🔧 Ejecutando configuración completa..."
    compile
    create_hostfile
    check_ssh
    echo ""
    echo "⚠️  PASOS SIGUIENTES:"
    echo "  1. Edita hosts.txt con tus nodos"
    echo "  2. Configura SSH sin contraseña"
    echo "  3. Ejecuta: $0 test"
    echo "  4. Ejecuta: $0 distribute"
    echo "  5. Ejecuta: $0 run <archivo> <texto> <procesos>"
}

# ==================== ROUTER DE COMANDOS ====================
case "$1" in
    compile)
        compile
        ;;
    hostfile)
        create_hostfile
        ;;
    check-ssh)
        check_ssh
        ;;
    test)
        test_cluster
        ;;
    distribute)
        distribute
        ;;
    run)
        shift
        run "$@"
        ;;
    full-setup)
        full_setup
        ;;
    *)
        show_menu
        ;;
esac


# ==================== ARCHIVO ADICIONAL: monitor_cluster.sh ====================
# Crear script de monitoreo
cat > monitor_cluster.sh << 'MONITOR_EOF'
#!/bin/bash
# monitor_cluster.sh - Monitorear estado del cluster durante ejecución

echo "╔════════════════════════════════════════════════════════╗"
echo "║           MONITOR DE CLUSTER MPI                       ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

if [ ! -f hosts.txt ]; then
    echo "❌ No existe hosts.txt"
    exit 1
fi

while true; do
    clear
    echo "🖥️  ESTADO DEL CLUSTER - $(date)"
    echo "═════════════════════════════════════════════════════════"
    echo ""
    
    # Leer hosts
    while read line; do
        [[ "$line" =~ ^#.*$ ]] && continue
        [[ -z "$line" ]] && continue
        
        host=$(echo $line | awk '{print $1}')
        slots=$(echo $line | grep -o 'slots=[0-9]*' | cut -d= -f2)
        
        if [ "$host" == "localhost" ]; then
            host_display="LOCAL"
        else
            host_display=$host
        fi
        
        echo -n "📍 $host_display ($slots slots): "
        
        if [ "$host" == "localhost" ]; then
            # Verificar procesos locales
            procs=$(ps aux | grep des_bruteforce_dynamic | grep -v grep | wc -l)
            cpu=$(top -bn1 | grep "Cpu(s)" | awk '{print $2}' | cut -d% -f1)
            echo "✅ $procs procesos activos | CPU: $cpu%"
        else
            # Verificar nodo remoto
            if ssh -o ConnectTimeout=2 $host "exit" 2>/dev/null; then
                procs=$(ssh $host "ps aux | grep des_bruteforce_dynamic | grep -v grep | wc -l")
                cpu=$(ssh $host "top -bn1 | grep 'Cpu(s)' | awk '{print \$2}' | cut -d% -f1")
                echo "✅ $procs procesos activos | CPU: $cpu%"
            else
                echo "❌ NO RESPONDE"
            fi
        fi
    done < hosts.txt
    
    echo ""
    echo "─────────────────────────────────────────────────────────"
    echo "Presiona Ctrl+C para salir"
    sleep 5
done
MONITOR_EOF

chmod +x monitor_cluster.sh

echo ""
echo "✅ Scripts creados:"
echo "   - setup_cluster.sh (este script)"
echo "   - monitor_cluster.sh (monitoreo en tiempo real)"