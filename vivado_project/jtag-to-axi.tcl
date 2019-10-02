variable GPIO_BASE     80000000
variable UART_BASE     80001000
variable UART_TX       [expr $UART_BASE + 4]
variable UART_STATUS   [expr $UART_BASE + 8]


variable TIMER_BASE    80002000
variable TIMER_CONTROL $TIMER_BASE+0
variable TIMER_COUNTER $TIMER_BASE+8

create_hw_axi_txn write_gpio_txn_55 [get_hw_axis hw_axi_1] -type WRITE -address $GPIO_BASE -data {00000055} -force
create_hw_axi_txn write_gpio_txn_AA [get_hw_axis hw_axi_1] -type WRITE -address $GPIO_BASE -data {000000AA} -force
for {set i 0} {$i < 10} {incr i} {

    run_hw_axi write_gpio_txn_55
    after 250
    run_hw_axi write_gpio_txn_AA
    after 250

}

set STRING1 {H e l l o , " " W o r l d ! "\r" "\n"}
foreach x $STRING1 {	;# Now loop and print...
    set c [format %8.8X [scan $x %c]]
    create_hw_axi_txn write_uart_txn [get_hw_axis hw_axi_1] -type WRITE -address $UART_TX -len 1 -data $c -force
    run_hw_axi write_uart_txn
}

# At this point, we know the PL is up and working...
#create_hw_axi_txn read_timer_txn [get_hw_axis hw_axi_1] -type read -address $TIMER_BASE -size 32 -force
#run_hw_axi read_timer_txn
