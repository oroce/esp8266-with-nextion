#include <Arduino.h>
#include <math.h>
#define arr_len(x) (sizeof(x) / sizeof(*x))
struct Unit
{
  long max;
  long value;
  String name;
  String prev;
};

Unit units[6] = {
    {2760000, 60000, "minute", "a minute ago"},
    {72000000, 3600000, "hour", "an hour ago"},
    {518400000, 86400000, "day", "yesterday"},
    {2419200000, 604800000, "week", "last week"},
    {28512000000, 2592000000, "month", "last month"},
    {INFINITY, 31536000000, "year", "last year"},
};


String format(long diff, long divisor, String unit, String prev) {
  long val = floor(diff / divisor);
  if (val <= 1) {
    return prev;
  }
  return String(val) + " " + String(unit) + "s ago";
}

String ago(long diff) {

  if (diff < 60000)
  { // less than a minute
    return "just now";
  }

  for (int i = 0; i < arr_len(units); i++)
  {
    if (diff < units[i].max)
    {
      return format(diff, units[i].value, units[i].name, units[i].prev);
    }
  }
}


