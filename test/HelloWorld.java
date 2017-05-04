package test;
import java.util.Random;



public class Foo
{
	int number;
}

public class HelloWorld {

	public static void main(String[] args) {
	       

		int N = 100000;
	    	Random randomGenerator = new Random();
	    	int numbers[] = new int[N];
	        for(int i = 0 ; i < N ; i++)
	        {
	        	numbers[i] = randomGenerator.nextInt(100000);
	        }
	       
		//assign first element of an array to largest and smallest
		int smallest = numbers[0];
		int largest = numbers[0];
	       
		for(int i=1; i< numbers.length; i++)
		{
		        if(numbers[i] > largest)
		                largest = numbers[i];
		        else if (numbers[i] < smallest)
		                smallest = numbers[i];
		       
		}
	       
		System.out.println("Largest Number is : " + largest);
		System.out.println("Smallest Number is : " + smallest);
	} 
}

