ipsec look
: ==== cut ====
ipsec auto --status
: ==== tuc ====
grep 'Result using RFC 3947' /tmp/pluto.log
if [ -n "`ls /tmp/core* 2>/dev/null`" ]; then echo CORE FOUND; mv /tmp/core* OUTPUT/; fi
if [ -f /sbin/ausearch ]; then ausearch -r -m avc -ts recent ; fi
: ==== end ====