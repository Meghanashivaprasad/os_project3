README

To compile the program, follow the below steps:
	$ gcc *.c -o main

After compiling, execute the executable:
	$ ./main numOfCustomers
		where, numOfCustomers is no of customer per queue for each seller types(ex: 5, 10, 15)

For Example:
	To simulate the sale of tickets with five customers arriving at random times for each seller's line, we can use the following command:
	$ ./main 5

Output Format:

	Timestamp SellerTypeSellerNumber / CustomerNumber Seller's Operation
	Average Statistics for TAT, RT, Throughput for Each Seller
	Seat Chart for all Tickets sold
