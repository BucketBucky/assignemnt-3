# assignemnt-3

python3 sql_server.py
mvn exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.StompServer" -Dexec.args="7777 tpc" 
mvn exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.StompServer" -Dexec.args="7777 reactor"
./bin/StompWCIClient 127.0.0.1 7777


login 127.0.0.1:7777 alice 1234
login 127.0.0.1:7777 bob 5679
join Germany_Japan
report data/events1_meni.json
summary Germany_Japan bob alice_report.txt // in alices terminal
exit Germany_Japan