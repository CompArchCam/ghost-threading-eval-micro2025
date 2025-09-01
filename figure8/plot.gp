set terminal pdfcairo enhanced font "Arial,12" size 14,5 #background rgb "gray90"
set bmargin 7
set tmargin 2.5
set lmargin at screen 0.04
set rmargin at screen 0.99

my_input = "speedup.csv"
set output "figure8.pdf"

set datafile separator ","
set style data histogram
set style histogram cluster gap 2
set style fill solid 0.8 border rgb "black" 
set boxwidth 0.7 

set style line 100 dt (10, 10, 10, 10) lc rgb "gray0" lw 1
set style line 101 dt (6, 6, 6, 6) lc rgb "gray" lw 1
set grid mytics ytics ls 100, ls 101

set yrange [-0.2:1.4]

set xtics rotate by 50 font ",18" right offset 2,0
set xtics scale 0
set ytics font ",18" scale 0 (\
     sprintf("%.1f", -0.2+1) -0.2, \
     sprintf("%.1f", 0+1) 0, \
     sprintf("%.1f", 0.2+1) 0.2, \
     sprintf("%.1f", 0.4+1) 0.4, \
     sprintf("%.1f", 0.6+1) 0.6, \
     sprintf("%.1f", 0.8+1) 0.8, \
     sprintf("%.1f", 1+1) 1, \
     sprintf("%.1f", 1.2+1) 1.2, \
     sprintf("%.1f", 1.4+1) 1.4, \
     sprintf("%.1f", 1.6+1) 1.6)
set ylabel "Speedup over Baseline" font ",18" offset -2,0

# Adjust the legend
set key top vertical left width -3 height 0.5 font ",18" reverse Left box
set key inside opaque offset 1,-1

set object 1 rectangle from screen -0.1,-0.1 to screen 1.1,1.1 fc rgb "white" behind

# bold the y=1 line
set arrow from graph 0, first 0 to graph 1, first 0 nohead lw 1.5 lc rgb "black"

# Labels for each dataset
swpf_label = "Software Prefetching"
homp_label = "SMT Parallelization via OpenMP"
ghost_label = "Ghost Threading"

start_row = '1'
end_row = '37'

# Plotting the data with color differentiation
plot my_input using ($3 - 1):xtic(($2 == 1) ? "{/:Bold ".stringcolumn(1)."}" : stringcolumn(1)) \
     every ::start_row::end_row title swpf_label lc rgb "orange", \
     '' using ($4 - 1) every ::start_row::end_row title homp_label lc rgb "#408000", \
     '' using ($5 - 1) every ::start_row::end_row title ghost_label lc rgb "web-blue" , \
     '' using ($0 - 0.2):(1.4):($3 >= 2.4 ? sprintf("{%0.2f}",$3):"") \
     every ::start_row::end_row with labels font ",16" offset 0,1.3 rotate by 45 notitle, \
     '' using ($0 + 0.0):(1.4):($4 >= 2.4 ? sprintf("{%0.2f}",$4):"") \
     every ::start_row::end_row with labels font ",16" offset 0,1.3 rotate by 45 notitle, \
     '' using ($0 + 0.3):(1.4):($5 >= 2.4 ? sprintf("{%0.2f}",$5):"") \
     every ::start_row::end_row with labels font ",16" offset 0,1.3 rotate by 45 notitle, \
     '' using ($0 + 0):(0):((stringcolumn(1) eq "kangaroo" || stringcolumn(1) eq "is") ? sprintf("x") : "") \
     every ::start_row::end_row with labels font ",14" offset 0,0.5 notitle
