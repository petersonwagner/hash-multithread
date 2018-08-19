# Hash Multithread

Esse trabalho tem como objetivo implementar uma hash multi-threaded criptografada com as operações de inserção e busca em C, usando a biblioteca OPENSSL e Pthreads. As requisições de busca e inserção são enviadas para o programa via Unix Sockets.

Na Tabela Hash, as eventuais colisões são resolvidas com árvores AVL para ter inserções e buscas em log n. A Hash é estruturada em um array de 1.000.000 de elementos sendo que cada elemento é um ponteiro para uma árvore. Cada nodo da árvore contém a chave, o nome criptografado, o telefone criptografado e os ponteiros para as sub-árvores. Para que as operações sejam multi-threaded, há um mutex para cada entrada da tabela que as operações de get e put disputam o seu acesso caso haja colisão e acesso simultâneo.

### Implementação
O programa principal se conecta com os sockets para obter as requisições, inicializa os semáforos, mutexes e estrutura de dados e cria 8 threads de inserção e 8 threads de busca e depois espera pelo término dessas threads.

### Threads de Busca
Cada thread de busca tem seu próprio buffer de mensagens e variáveis locais. A cada mensagem processada, é veriﬁcado se o relógio lógico da mensagem de get for menor que o menor relógio lógico de todos os puts, essa thread é bloqueada até que algum put a libere novamente com um signal no semáforo.

### Threads de Inserção
Cada thread de inserção também tem seu próprio buffer de mensagens e variáveis locais. Depois de inserir na hash, cada thread tenta ganhar acesso ao mutex de "ressuscitador de gets", se não ganhar o acesso ao mutex, a thread simplesmente continua sua execução adiante. Se a thread conseguir acesso a esse mutex, ela atualiza a variável smallest put clock. Depois disso, veriﬁca se tem algum get bloqueado e se tiver, veriﬁca se ele pode voltar a execução novamente comparando o seu clock com o menor dos relógios dos puts. O pseudo-código 2 resume a funcionalidade.
