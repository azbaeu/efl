TEXT="Evas Filters"
FONT="Sans"
SIZE=50
FILTER="
buffer : fat (alpha);
grow (8, dst = fat);
blur (12, src = fat, color = darkblue);
blur (4, color = cyan);
blend ();
"
