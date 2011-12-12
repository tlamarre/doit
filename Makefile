all: server lazerChatServer lazerChatTalk lazerChatListen

lazerChatServer: lazerChatServer.o userDictionary.o
	gcc -lpthread -o lazerChatServer lazerChatServer.o userDictionary.o

lazerChatTalk: 
	gcc -o lazerChatTalk lazerChatTalk.c

lazerChatListen:
	gcc -lpthread -o lazerChatListen lazerChatListen.c

client: royalty_client.o fileDictionary.o jobDictionary.o
	 gcc -o client royalty_client.o fileDictionary.o jobDictionary.o

%.o: %.c 
	gcc -c -o $@ $<

