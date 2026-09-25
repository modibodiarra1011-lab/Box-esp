# Protocole Worker

## Découverte

`HELLO|MAC|VERSION|IP`

## Attribution

`ASSIGN|ID|MASTER_IP`

## Heartbeat

`HB|ID|MAC|STATE|PROGRESS|UPTIME|FREE_HEAP|IP|JOB`

## Configuration AP

`APCFG|SSID_URLENCODED|PASSWORD_URLENCODED`

## Journal

`LOG|WID|MILLIS|MESSAGE`

Les workers W1/W2/W3 partagent le même firmware. Le MASTER décide de l'affectation et du scheduling.
