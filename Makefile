all: server lazerChatServer lazerChatTalk lazerChatListen

lazerChatServer: lazerChatServer.o userDictionary.o
	gcc -lpthread -o lazerChatServer lazerChatServer.o userDictionary.o

lazerChatTalk: 
	gcc -o lazerChatTalk lazerChatTalk.c

lazerChatListen:
	gcc -lpthread -o lazerChatListen lazerChatListen.c

%.o: %.c 
	gcc -c -o $@ $<

