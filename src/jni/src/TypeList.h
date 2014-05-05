struct TypeNil{
};

template <typename T1, typename T2>
struct TypeCons{
	template <typename T3>
	TypeCons<TypeCons<T1, T2>, T3> operator+(const TypeCons<TypeNil, T3> &){
		return TypeCons<TypeCons<T1, T2>, T3>();
	}
	template <typename T3, typename T4>
	TypeCons<TypeCons<T3, T4>, T2> operator=(const TypeCons<T3, T4> &){
		return TypeCons<TypeCons<T3, T4>, T2>();
	}
};

/*
How to construct this list:

	Use this defines (you may rename them if you want):
	#define NODE(x) TypeCons<TypeNil, x>()
	//#define cons +
	//#define cons =
	Which type_cons you use depends on how you want to write the list:
	With +, the list NODE(T1) type_cons NODE(T2) will be iterated in
	order T2, T1. With =, it will be iterated in order T1, T2.
	To construct the list do:
	auto x = NODE(T1) cons NODE(T2) cons NODE(T3) ...;
	Notes:
	* Storing this type in a variable without type inference is rather painful.
	  If the compiler doesn't support type inference, I recommend directly
	  passing the object to a template function.
	* Each element adds at least one more level of template depth. Compilers
	  usually set a limit of around 1024 levels of template depth. Also,
	  deeply-nested templates tend to require a lot of memory to compile.

How to iterate this list:

	For example,
	
	void Print(const TypeNil &){}

	template <typename T1, typename T2>
	void Print(TypeCons<T1, T2>){
		std::cout <<typeid(T2).name()<<std::endl;
		Print(T1());
	}

	Print(NODE(T1) cons NODE(T2) cons NODE(T3) ...);
*/
