Objetivo
    O objetivo deste projeto é implementar o cenário de rede proposto com recurso ao GNS3, com todas as configurações necessárias ao suporte do encaminhamento entre as redes, e criar um servidor, que permita ser gerido por uma consola de administração, e terminais de operações para a simular um sistema de transações de bolsa.

Introdução
    A realização do projeto consistiu em quatro etapas: Implementar o cenário de rede; criar o servidor de bolsa; criar a consola de administração; criar o terminal de operações; e implementação do multicast.

Criação do servidor de bolsa
		Começa por ler o ficheiro de configurações, cria a shared memory e um novo processo para gerir a consola de administração. De seguida, cria uma thread para enviar o feed de cotações aos clientes por multicast e, finalmente, começa a aceitar conexões de clientes. Para cada cliente que se conecta, uma nova thread é criada para comunicar com este, sendo que trocam mensagens com o formato "comando {argumentos}", através de TCP, com a finalidade do cliente poder aceder às diversas funcionalidades do servidor.
		Uso da shared memory
				É utilizada uma shared memory para o servidor de bolsa e o processo que gere a consola de administração partilharem os dados necessários ao bom funcionamento do servidor, incluindo dados sobre os diversos utilizadores e mercados. Para evitar corrupção de dados em acessos simultâneos, é utilizado um mutex para aceder à shared memory.

Criação da consola de administração
		Espera, inicialmente pela autenticação de um administrador através das suas credenciais, garantindo, depois, que nenhuma outra entidade sem ser este administrador consegue enviar comandos sem iniciar sessão. Para isto, guardam-se as informações sobre o endereço deste administrador e verifica-se, sempre, ao receber um novo comando, que este foi enviado por ele, bloqueando outras tentativa de intromissão na sessão. Quando este administrador sai da sessão (ao enviar o comando "QUIT"), este poderá iniciar novamente a sessão (mesmo sendo noutro dispositivo), aplicando os mesmos mecanismos de segurança aplicados ao primeiro.

Criação dos terminais de operações
		Para o uso do terminal o cliente tem de inserir o seu o username e password, e este, ao ser autenticado, recebe os mercados a que está permitido aceder. Após isso, é mostrado um menu com opções que este pode tomar: subscrever mercados; comprar/vender ações; ligar/desligar o feed de atualização de ações no ecrã; ver a informação da sua carteira e o saldo disponível; e terminar sessão.
		Para a subscrição de mercados, é necessário o cliente indicar que mercado quer subscrever, sendo que só é permitida a subscrição caso o mercado exista e o cliente o possa aceder.
		Já para a compra/venda de ações, é essencial que o cliente especifique a identificação da ação, o número de ações e o preço, sendo que só é ocorre a compra/venda caso a identificação da ação seja válida e se o cliente tiver acesso ao mercado da ação. No ato de compra, é também necessário que o valor de venda da ação naquele momento for igual ou inferior ao preço enviado e haver saldo disponível para a realização da mesma. No ato de venda, é também necessário que valor do comprador da ação naquele momento for igual ou superior ao preço enviado e que hajam as ações a vender na carteira do cliente.

Implementação do multicast
		Cada mercado corresponde a um grupo de multicast, que possui endereços da classe D do tipo 239.0.0.X, sendo X o id do mercado. A cada REFRESH_TIME segundos, uma thread do servidor de bolsa envia para todos os grupos de multicast informações sobre o respetivo mercado. Do lado do terminal de operações é criada uma nova thread para cada mercado subscrito, responsável por receber as informações sobre o grupo de multicast desse mercado, cujo endereço é fornecido pelo servidor aquando da subscrição. Se o feed estiver desligado, este terminal deixará de receber estas informações até o feed voltar a estar ativo.

Conclusão
		Neste projeto criou-se, com recurso ao GNS3, uma simulação de um sistema de transações de bolsa, aplicando temas abordados nesta unidade curricular, como subnetting, NAT, multicast, sockets TCP e UDP.
