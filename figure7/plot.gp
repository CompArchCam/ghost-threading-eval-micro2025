set terminal pdfcairo enhanced font "Arial,12" size 14,5
set bmargin 9
set tmargin 1.3
set lmargin at screen 0.047
set rmargin at screen 0.99

my_input = "energy_saving.csv"
set output "figure7.pdf"

set datafile separator ","
set style data histogram
set style histogram cluster gap 2
set style fill solid 0.8 border rgb "black" 
set boxwidth 0.7 

set style line 100 dt (10, 10, 10, 10) lc rgb "gray0" lw 1
set style line 101 dt (6, 6, 6, 6) lc rgb "gray" lw 1
set grid mytics ytics ls 100, ls 101

set yrange [-0.15:0.45]

set xtics rotate by 50 font ",18" right offset 2,0
set xtics scale 0
set ytics font ",18" scale 0 (\
     "-15%%" -0.15, \
     "0%%" 0, \
     "15%%" 0.15, \
     "30%%" 0.3, \
     "45%%" 0.45, \
)
set ylabel "Energy Saving over Baseline" font ",18" offset -2.7,0

# Adjust the legend
# set key top horizontal center width 10 font ",16"
# set key outside offset -6,0
set key top vertical left width -3 height 0.5 font ",18" reverse Left box
set key inside opaque offset 1,0

set arrow from graph 0, first 0 to graph 1, first 0 nohead lw 1.5 lc rgb "black"

# Labels for each dataset
swpf_label = "Software Prefetching"
homp_label = "SMT Parallelization via OpenMP"
htpf_label = "Ghost Threading"

start_row = '1'
end_row = '37'
# Plotting the data with color differentiation
plot my_input using ($3):xtic(($2 == 1) ? "{/:Bold ".stringcolumn(1)."}" : stringcolumn(1)) \
     every ::start_row::end_row title swpf_label lc rgb "orange", \
     '' using ($4) every ::start_row::end_row title homp_label lc rgb "#408000", \
     '' using ($5) every ::start_row::end_row title htpf_label lc rgb "web-blue" , \
     '' using ($0 + 0.27):(0.45):($5 >= 0.45 ? sprintf("{%i%%}",($5)*100) : "") \
     every ::start_row::end_row with labels font ",16" offset 0,0.7 notitle, \
     '' using ($0 + 0):(0):((stringcolumn(1) eq "kangaroo" || stringcolumn(1) eq "is") ? sprintf("x") : "") \
     every ::start_row::end_row with labels font ",14" offset 0,0.5 notitle
