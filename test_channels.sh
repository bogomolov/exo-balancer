#!/bin/zsh

if [[ z$1 == "zforce" ]]; then
  FORCED_BALANCE="force"
fi

if [[ -z $FORCED_BALANCE ]]; then
  echo 'Started at '`date +"%F %T"`
else
  echo 'Started at '`date +"%F %T"`' with force balancing'
fi

BALANCER_DIR="/etc/network/exo-balancer"

. $BALANCER_DIR/functions.inc

# Загрузка конфигов
load_cfg

# Загрузка переменных с цветами для раскраски вывода
load_colors

# Цикл по переменным, имена которых составлены из ISP и цифры
for ((i = 1; i < 10; i += 1)); do 
  if [[ ! -z $ISP_NAME[$i] ]]; then
    # Вспомогательные переменные
    IFACE="$ISP_IFACE[$i]"
    ADDR="$ISP_ADDR[$i]"
    INAME="$ISP_NAME[$i]"
    GW="$ISP_GW[$i]"
    DT=`date +"%F %R"`
    
    verb ${IFACE} ${ADDR} ${INAME}
  
    # Network interface test and put results into arrays
    iface_get_status ${IFACE} ${GW} 8.8.8.8
    IFACE_STATUS[$i]=${RES}
    IFACE_LOSSES[$i]=${LOSSES}

    echo $IFACE_STATUS[$i] > "${BALANCER_DIR}/state/"${IFACE}".state"
    echo "[${DT}] $IFACE_STATUS[$i]" >> "${BALANCER_DIR}/logs/"$IFACE".log"
  fi
done

#Result table
echo "---------------------------------------------------------------"
# Header
iface_test_header
# Data
for ((i = 1; i < 10; i += 1)); do 
  if [[ ! -z $ISP_NAME[$i] ]]; then
    iface_show_result $ISP_NAME[$i] $ISP_IFACE[$i] $ISP_ADDR[$i] "$IFACE_STATUS[$i]" "$IFACE_LOSSES[$i]"
  fi
done

. ${BALANCER_DIR}/balance.sh
balance

echo 'Finished at '`date +"%F %T"`
