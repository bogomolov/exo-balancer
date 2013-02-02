#!/bin/zsh

#. $BALANCER_DIR/functions.inc

function balance {
    echo start balancer
    get_channel_list
    active_channels="$RES"
    if [[ -z $FORCED_BALANCE ]]; then
      last_active_channels=`grep "Route changed to" $BALANCER_DIR/logs/balancer.log | tail -n 1 | sed "s/.* \([1-9]*\)$/\1/g"`
    else
      last_active_channels="00"
      echo "Force balancing"
    fi
    echo "Active channels: "$active_channels
    echo "Last channels: "$last_active_channels
    # test if file with current rules exists
    if [[ -e $BALANCER_DIR/scenarios/$active_channels.rules ]]; then
        if [[ "$active_channels" = "$last_active_channels" ]]; then
	    verb `date +"[%F %T]"` "New route same as last route. No changes" 
        else
            . $BALANCER_DIR/scenarios/before.rules
            . $BALANCER_DIR/scenarios/$active_channels.rules
            . $BALANCER_DIR/scenarios/after.rules
            before
            change_route
            after
	    echo `date +"[%F %T]"` "Route changed to $active_channels" >> $BALANCER_DIR/logs/balancer.log
        fi
    else
        echo `date +"[%F %T]"` "ERR: file with rule $active_channels does not exists! Can't change route!" >> $BALANCER_DIR/logs/balancer.log
    fi

}

reload_dns()
{
	service bind9 restart
}
reload_sip()
{
	asterisk -rx "sip reload"
}

mode_0_uno()
{
	log down
	clean
	voip_dns_route
	/etc/network/ifdnup ${ISP0_I}
	reload_dns
	reload_sip
}
mode_1_uno()
{
	log up
	clean
	ip route add default via ${ISP0_G} dev ${ISP0_I}
	iptables -F -t nat
	iptables -t nat -A POSTROUTING -s $LAN0_N/$LAN0_M -o $ISP0_I -j MASQUERADE
	iptables -t nat -A POSTROUTING -s $LAN1_N/$LAN1_M -o $ISP0_I -j MASQUERADE
	voip_dns_route
	reload_dns
	reload_sip
}

mode_0_duo()
{
	log none s
	clean
	voip_dns_route
	/etc/network/ifdnup ${ISP0_I} ${ISP1_I}
	reload_dns
	reload_sip
}
mode_1_duo()
{
	log 1st
	clean
	voip_dns_route
	ip route add default via ${ISP0_G} dev ${ISP0_I}
	/etc/network/ifdnup ${ISP1_I}
	reload_dns
	reload_sip
}
mode_2_duo()
{
	log 2nd
	clean
	voip_dns_route
	ip route add default via ${ISP1_G} dev ${ISP1_I}
	/etc/network/ifdnup ${ISP0_I}
	reload_dns
	reload_sip
}
mode_3_duo()
{
	log both s
	clean
	
	voip_dns_route

	ip route add default scope global \
		nexthop dev ${ISP0_I} via ${ISP0_G} weight ${ISP0_W} \
		nexthop dev ${ISP1_I} via ${ISP1_G} weight ${ISP1_W} 

	/etc/network/ifdnup ${ISP0_I} ${ISP1_I}

	reload_dns
	reload_sip
}

#. /etc/network/config

#mode=duo
#case $1 in
#	0|1|2|3)
#	0|2)
#		if [ $RESERVE == "yes" ]; then
#			mode=uno
#			mode_1_uno
#		else
#			mode_$1_$mode
#		fi

#		mode_1_uno
#	;;
#esac
