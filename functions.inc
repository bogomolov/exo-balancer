#!/bin/zsh

# -*-Shell-script-*-
#
# functions     This file contains functions to be used by most or all
#               shell scripts in the exo-balancer
#

RES=""

function test_case {
    echo "losses is $1 %"
    case ${1} in
      (100)
      RES="NO GATE"
      ;;
      ([6-9]?)
      RES="BAD GW"
      ;;
      (*)
      RES="GW UP"
      ;;
    esac
    echo $RES
}

function verb {
  if [[ $VERBOSE = "yes" ]]; then
    echo "$1"
  fi
}

function load_cfg {
  # read main config
  . $BALANCER_DIR/config/balancer.conf
  
  #declare array of ISP channels parameters
  #typeset -A ISP_NAME
  #typeset -A ISP_IFACE
  #typeset -A ISP_ADDR
  #typeset -A ISP_MASK
  #typeset -A ISP_GW
  #typeset -A ISP_UDP
  #typeset -A ISP_RSRV
  
  #set values
  for i in $BALANCER_DIR/config/isp[1-9].conf; do
    # prefix and index by configuration filename
    prefix=`echo $i | sed "s/.*config\/\(.*\)\([0123456789]\)\.conf/\U\1/"`
    index=`echo $i | sed "s/.*config\/\(.*\)\([0123456789]\)\.conf/\U\2/"`
    #make variable list file
    cat $i | sed "s/\(.*\)=\(.*\)/${prefix}_\1[${index}]=\2/" >> $BALANCER_DIR/tmp/compound.vars
    if [[ "$BALANCER_DEBUG" == "yes" ]]; then
      cat $BALANCER_DIR/tmp/compound.vars
    fi
    . $BALANCER_DIR/tmp/compound.vars
  done
  
  if [[ $VERBOSE = "yes" ]]; then
    cat $BALANCER_DIR/tmp/compound.vars
  fi
  
  rm $BALANCER_DIR/tmp/compound.vars

  verb "Config loaded..."
}

function load_colors {
  red=$(tput setaf 1)
  green=$(tput setaf 2)
  orange=$(tput setaf 3)
  blue=$(tput setaf 4)
  purple=$(tput setaf 5)
  white=$(tput setaf 7)
  normal=$(tput sgr0)
  verb "Colors loaded..."
}

function iface_status {
  iface="$#"
  echo $iface test
}

function iface_test_header {
  printf "%-10s %-10s %-20s %-7s %12s\n" "Provider" "Iface" "Address" "Status" "Lossess  "
  printf "%s\n" "---------------------------------------------------------------"
}

function iface_get_status {
  # Аргументы:
  # 1 - наименование интерфейса
  # 2 - адрес шлюза
  # 3 - внешний адрес для теста интерфейса

  RES=""
  LOSSES=""
  # проверка наличия интерфейса
  if_exists=`ifconfig $1 2>&1 | grep "Device not found"`
  if [[ ! -z $if_exists ]]; then
    RES="NOT FOUND"
    verb $RES
    return
  fi
  # проверка статуса интерфейса
  if_updown=`ifconfig $1 | grep UP`
  if [[ -z $if_updown ]]; then
    RES="DOWN"
    verb ${RES}
    return
  fi
  # проверка статус настройки сети
  if_conf=`ifconfig $1 | grep inet`
  if [[ -z $if_conf ]]; then
    RES="NOT CONF"
    verb ${RES}
    return
  fi
  
  verb $1
  verb "iface $1 status is $iface_status"
  
  ping_cnt="5"
  # проверка пинга до шлюза (если есть)
  if [[ ! -z $2 ]]; then
    gw_result=`ping -c $ping_cnt $2 | grep -o "[0-9]*\% packet loss, time [0-9]*ms"`
    gw_losses=`echo $gw_result | sed "s/\([0-9]*\)\% packet loss, time \([0-9]*\)ms/\1/"`
    gw_wait=`echo $gw_result | sed "s/\([0-9]*\)\% packet loss, time \([0-9]*\)ms/\2/"`

    LOSSES=${gw_losses}

    case ${gw_losses} in
      (100)
      RES="NO GW"
      return
      ;;
      ([6-9]?)
      RES="BAD GW"
      return
      ;;
      (*)
      RES="GW UP"
      ;;
    esac
    verb "GW wait for ${ping_cnt} pings: ${gw_wait} ms, ${gw_losses}% losses"
    
  fi
  
  # проверка пинга до внешнего адреса
  if [[ "$3" != "" ]]; then
    outside_result=`ping -c $ping_cnt -W 2 $2 | grep -o "[[:alnum:]]* received" | grep -o "^[[:alnum:]]*"`
    verb "PING OUTSIDE RESULT: $outside_result of $ping_cnt"
  fi
  
}

function write_iface_status {
  # Аргументы:
  # 1 - 
}
function iface_show_result {
  # Аргументы:
  # 1 - наименование провайдера
  # 2 - наименование интерфейса
  # 3 - адрес интерфейса
  # 4 - результат
  # 5 - % losses

    case $4 in
      (NOT FOUND)
      COLOR="${orange}"
      ;;
      (DOWN)
      COLOR="${red}"
      ;;
      (UP)
      COLOR="${green}"
      ;;
      (OFF)
      COLOR="${red}"
      ;;
      (BAD)
      COLOR="${purple}"
      ;;
    esac

  printf "%-10s %-10s %-20s %-7s %12s\n" $1 \($2\) \($3\) "${COLOR} $4 ${normal}" "($5%)"

}

function get_ch_no_reserve_udp {
    active_channels=""
    active_udp_channels=""
    for ((i = 1; i<10; i+=1)); do
        if [[ ( ! -z $ISP_NAME[$i] ) && ( $IFACE_STATUS[$i] = $GOOD_STATUS ) && ( $ISP_RSRV[$i] = "no" ) && ( $ISP_UDP[$i] = "yes" ) ]]; then
                active_channels="$active_channels$i"
        fi
    done
    RES=$active_channels
}

function get_ch_no_reserve_all {
    active_channels=""
    active_udp_channels=""
    for ((i = 1; i<10; i+=1)); do
        if [[ (! -z $ISP_NAME[$i] ) && ( $IFACE_STATUS[$i] = $GOOD_STATUS ) && ( $ISP_RSRV[$i] = "no" ) ]]; then
                active_channels="$active_channels$i"
        fi
    done
    RES=$active_channels
}

function get_ch_active_all {
    active_channels=""
    active_udp_channels=""
    for ((i = 1; i<10; i+=1)); do
        if [[ ( ! -z $ISP_NAME[$i] ) && ( $IFACE_STATUS[$i] = $GOOD_STATUS ) ]]; then
                active_channels="$active_channels$i"
        fi
    done
    RES=$active_channels
}

function get_channel_list {
    get_ch_no_reserve_udp
    active_udp_channels="$RES"
    if [[ ! -z $active_udp_channels ]]; then
      #if there are any no reserve with UDP then use current active channels
      get_ch_no_reserve_all
      RES="$RES"
    else
      #if there are only reserve channels with UDP then use reserve channels too
      get_ch_active_all
      RES="$RES"
      if [[ -z $RES ]]; then
        verb "ERR: No active channels!!!"
      fi
    fi
}
