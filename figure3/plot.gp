set terminal pdfcairo enhanced font "Arial,12" size 10,5 
# set bmargin 7
# set tmargin 2.5
# set lmargin at screen 0.3
# set rmargin at screen 0.99

my_input = "speedup.csv"
set output "figure3.pdf"

set datafile separator ","
set style data histogram
set style histogram cluster gap 3
set style fill solid 0.8 border rgb "black" 
set boxwidth 0.7 

set style line 100 dt (10, 10, 10, 10) lc rgb "gray0" lw 1
set style line 101 dt (6, 6, 6, 6) lc rgb "gray" lw 1
set grid mytics ytics ls 100, ls 101

set yrange [0:1.5]

set autoscale xfixmin
set autoscale xfixmax
set xrange [*:*] 
set offset -0.4,-0.5,0,0  # Adds extra space on left and right

set xtics font ",24" center
set xtics scale 0
set ytics font ",24" scale 0 (\
     sprintf("%.1f", 0+1) 0, \
     sprintf("%.1f", 0.2+1) 0.2, \
     sprintf("%.1f", 0.4+1) 0.4, \
     sprintf("%.1f", 0.6+1) 0.6, \
     sprintf("%.1f", 0.8+1) 0.8, \
     sprintf("%.1f", 1+1) 1, \
     sprintf("%.1f", 1.2+1) 1.2, \
     sprintf("%.1f", 1.4+1) 1.4, \
     sprintf("%.1f", 1.6+1) 1.6, \
     sprintf("%.1f", 1.8+1) 1.8, \
     sprintf("%.1f", 2+1) 2, \
     sprintf("%.1f", 2.2+1) 2.2, \
     sprintf("%.1f", 2.4+1) 2.4, \
     sprintf("%.1f", 2.6+1) 2.6 \
)
set ylabel "Speedup over Baseline" font ",24" offset -2,0

# Adjust the legend
set key top vertical center width -3 height 0.5 font ",22" reverse Left box
set key inside opaque offset 5,-1

# set object 1 rectangle from screen -0.1,-0.1 to screen 1.1,1.1 fc rgb "white" behind

# bold the y=1 line
set arrow from graph 0, first 0 to graph 1, first 0 nohead lw 1.5 lc rgb "black"

# Labels for each dataset
swpf_label = "Software Prefetching"
homp_label = "Parallelization via OpenMP"
htpf_label = "Ghost Threading"

start_row = '1'
end_row = '4'

# Plotting the data with color differentiation
plot my_input using ($2 - 1):xtic(1) every ::start_row::end_row title swpf_label lc rgb "orange", \
     '' using ($3 - 1) every ::start_row::end_row title homp_label lc rgb "#408000", \
     '' using ($4 - 1) every ::start_row::end_row title htpf_label lc rgb "web-blue" 
