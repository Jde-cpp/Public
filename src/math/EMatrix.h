#pragma once
#include <random>
#include <unordered_set>
#include "Eigen/Dense" ///home/duffyj/code/libraries/eigen/
#include "Eigen/Sparse"

namespace Jde
{
	template<int Rows=-1,int Cols=-1>
	using M = Eigen::Matrix<double,Rows,Cols>;
	namespace EMatrix
	{
		using Eigen::Matrix;
		using Eigen::MatrixXd;
		using Eigen::VectorXi;
		using Eigen::SparseMatrix;

		template<int Rows,int Cols>
		using MatrixD = Matrix<double,Rows,Cols>;

		template<typename T=float,int Rows=-1,int Cols=-1>
		using MPtr = up<Matrix<T,Rows,Cols,0>>;
		template<typename T=float,int Rows=-1,int Cols=-1>
		using MSharedPtr = std::shared_ptr<Matrix<T,Rows,Cols,0>>;
		//template<typename T,int Rows,int Cols>
		//using MatrixPtr = up<Matrix<T,Rows,Cols>>;

		template<typename T>
		using SparseT = Eigen::SparseMatrix<T>;
		//using Sparse = Eigen::SparseMatrix<float>;
		using SparsePtr = up<Eigen::SparseMatrix<float>>;
		using SparseV = Eigen::SparseVector<float>;
		using SparseVPtr = up<Eigen::SparseVector<float>>;

		template<int Rows>
		using VectorD = Matrix<double,Rows,1>;
		template<typename T=float,int Rows=-1>
		using Vector = Matrix<T,Rows,1>;
		/*template<typename T=float,int Rows=-1>
		using V = Matrix<T,Rows,1>;*/
		template<typename T=float,int Rows=-1>
		using VPtr = up<Matrix<T,Rows,1>>;
		template<typename T,int Cols>
		using RowVector = Matrix<T,1,Cols>;
		template<typename T,int Cols>
		using RowVectorPtr = up<Matrix<T,1,Cols>>;
		template<int Cols>
		using RowVectorD = Matrix<double,1,Cols>;
		template<int Cols>
		using Row = Matrix<double,1,Cols>;
		template<int Cols>
		using RowPtr = up<Row<Cols>>;

#pragma warning( push )
#pragma warning( disable : 4251)
#pragma region ForEachCell
		template<typename T,int64_t Rows,int64_t Cols>
		void ForEach( const Eigen::Matrix<T,Rows,Cols,0>& matrix, const std::function<void(size_t,size_t,T)>& function );
		template<typename T,int64_t Rows>
		void ForEach( const Eigen::Matrix<T,Rows,1>& vector, const std::function<void(size_t,T)>& function );
		template<typename T,int64_t Rows,int64_t Cols>
		void ForEachRef( Eigen::Matrix<T,Rows,Cols>& matrix, size_t j, const std::function<void(size_t,T&)>& function );
		template<typename T,int64_t Cols>
		void ForEachRowVector( const RowVector<T,Cols>& rowVector, const std::function<void(size_t,T)>& function );
		// template<typename T>
		// void ForEachValueRef( const SparseMatrix<T>& matrix, const std::function<void(int,int,T&)>& function, Stopwatch* pStopwatch=nullptr );
		template<typename T=float,int64_t Rows,int64_t Cols>
		void ForEachRowColumn( const Eigen::Matrix<T,Rows,Cols,0>& original, uint columnIndex, function<void(uint rowIndex,const T& value)> func );


		template<typename T>
		void TransformEachValue( SparseMatrix<T>& matrix, const std::function<void(int,int,T&)>& function );
		template<typename T>
		void TransformEachValue( Eigen::SparseVector<T>& matrix, const std::function<void(int,T&)>& function );

		//JDE_NATIVE_VISIBILITY void ForEachItem( const Eigen::VectorXd& vector, const std::function<void(size_t,double)>& function );
		//JDE_NATIVE_VISIBILITY void ForEachCell( Eigen::MatrixXd& matrix, std::function<void(size_t,size_t,double)> function );
#pragma endregion
#pragma region AssignEachCell
		template<typename T=float,int64_t Rows=-1,int64_t Cols=-1>
		void Assign( Eigen::Matrix<T,-1,-1>& toMatrix, const Eigen::Matrix<T,Rows,Cols,0>& fromMatrix, uint startingRowIndex=0 );
		template<typename T,size_t RowCount,size_t ColumnCount>
		void AssignEachCell( Eigen::Matrix<T,RowCount,ColumnCount,0>& matrix, std::function<T(T)> function );
		template<typename T,size_t RowCount,size_t ColumnCount>
		void AssignEachCellWithIndex( Eigen::Matrix<T,RowCount,ColumnCount>& matrix, std::function<T(T, size_t)> function );
#pragma endregion
		//JDE_NATIVE_VISIBILITY Eigen::MatrixXd Add( const Eigen::VectorXd& vector, double scaler/*, const std::function<double(double)>& function*/ );
		//JDE_NATIVE_VISIBILITY Eigen::MatrixXd Add( const Eigen::MatrixXd& matrix, double scaler );

		template<typename T>
		void AppendColumn( Matrix<T,-1,-1,0>& matrix, const Vector<T,-1>& column );

		template<typename T>
		MSharedPtr<T> AppendColumns( const Matrix<T,-1,-1,0>& matrix, const Matrix<T,-1,-1,0>& column );

		template<typename T,int Rows=-1>
		std::tuple<double,double,double> Auc( const Matrix<T,Rows,1>& actuals, const Matrix<T,Rows,1>& predictions );

		//JDE_NATIVE_VISIBILITY Eigen::VectorXd AverageColumnsOld( const Eigen::MatrixXd& matrix );
		template<typename T,int Rows=-1,int Cols=-1>
		Eigen::Matrix<T,1,Cols> AverageColumns( const Eigen::Matrix<T,Rows,Cols>& matrix );

		template<int Rows,int Cols, int Matrix2Rows>
		MPtr<double,Rows,Cols> BsxFun( const M<Rows,Cols>& matrix1, const M<Matrix2Rows,Cols>& matrix2, const std::function<double(double,double)>& binaryFunction );
		template<int Rows,int Cols, int Matrix2Rows>
		MPtr<double,Rows,Cols> BsxFunSubtract( const M<Rows,Cols>& matrix, const Row<Cols>& rowVector ){ return BsxFun<Rows,Cols,Matrix2Rows>( matrix, rowVector, [](double value1, double value2){return value1-value2;} ); }

		template<int Rows,int Cols, int Matrix2Rows>
		MPtr<double,Rows,Cols> BsxFunMultiply( const M<Rows,Cols>& matrix1, const M<Rows,Cols>& matrix2 ){ return BsxFun<Rows,Cols,Matrix2Rows>( matrix1, matrix2, [](double value1, double value2){return value1*value2;} ); }

		template<int Rows,int Cols>
		void BsxFunTransform( MatrixD<Rows,Cols>& matrix, const RowVectorD<Cols>& rowVector, const std::function<double(double,double)>& binaryFunction );
		template<int Rows,int Cols>
		void BsxFunTransformDivide( MatrixD<Rows,Cols>& matrix, const RowVectorD<Cols>& rowVector ){ BsxFunTransform<Rows,Cols>( matrix, rowVector, [](double value1, double value2){return value1/value2;} ); }
		template<int Rows,int Cols>
		void BsxFunTransformSubtract( MatrixD<Rows,Cols>& matrix, const RowVectorD<Cols>& rowVector ){ BsxFunTransform<Rows,Cols>( matrix, rowVector, [](double value1, double value2){return value1-value2;} ); }

		Eigen::MatrixXd BsxFunOld( const Eigen::MatrixXd& matrix1, const Eigen::MatrixXd& matrix2, std::function<double(double,double)> binaryFunction );

		//JDE_NATIVE_VISIBILITY Eigen::MatrixXd BsxFunMultiplyOld( const Eigen::MatrixXd& matrix1, const Eigen::MatrixXd& matrix2 );
		template<typename T, int Rows=-1, int Cols=-1>
		Eigen::Matrix<T,Rows,Cols> Centered( const Eigen::Matrix<T,Rows,Cols>& matrix, Eigen::Matrix<T,1,Cols>* pAverage=nullptr );
		template<typename T>
		up<Eigen::Matrix<T,-1,-1>> CorrelationCoefficient( const SparseMatrix<T>& sparse, Eigen::Matrix<T,-1,-1>* pExisting, size_t threadCount=1, Stopwatch* pStopwatch=nullptr, size_t columnCount=std::numeric_limits<size_t>::max(), const std::function<void(const Eigen::Matrix<float,-1,-1>&)>* saveFunction=nullptr );
		template<typename T, int Rows=-1, int Cols=-1>
		Eigen::Matrix<T,Cols,Cols> Covariance( const Eigen::Matrix<T,Rows,Cols>& matrix, Eigen::Matrix<T,1,Cols>* pAverage=nullptr );
		template<typename T>
		up<Eigen::Matrix<T,-1,-1>> Covariance( const SparseMatrix<T>& sparse, Eigen::Matrix<T,-1,-1>* pExisting, bool correlationCoefficient=false, size_t threadCount=1, Stopwatch* pStopwatch=nullptr, size_t columnCount=std::numeric_limits<size_t>::max(), const std::function<void(const Eigen::Matrix<float,-1,-1>&)>* saveFunction=nullptr );
		template<typename T, int Rows=-1, int Cols=-1>
		void ExportCsv( sv pszFileName, const Eigen::Matrix<T,Rows,Cols>& matrix, const vector<string>* pColumnNames );
		template<typename T, int64_t Rows, int64_t Cols>
		Eigen::Matrix<T,Rows,Cols>& ExpTransform( Eigen::Matrix<T,Rows,Cols>& matrix );
		       //Eigen::Matrix<T,Rows,Cols>& ExpTransform( Eigen::Matrix<T,Rows,Cols>& matrix )
		template<typename T>
		SparseMatrix<T>& ExpTransform( SparseMatrix<T>& matrix );

		template<typename T>
		size_t EqualCount( const Eigen::Matrix<T, Eigen::Dynamic, 1>& v1, const Eigen::Matrix<T, Eigen::Dynamic, 1>& v2 );
		template<typename T>
		size_t EqualCount( const Eigen::SparseVector<T>& v1, const Eigen::SparseVector<T>& v2 );
		template<typename T,int Rows=-1, int Cols=-1>
		size_t Count( const Eigen::Matrix<T,Rows, Cols>& vector, const T value );
		template<typename T>
		size_t Count( const Eigen::SparseVector<T>& vector, const T value );
		template<typename T, int Rows=-1>
		std::tuple<int64_t,int64_t,int64_t,int64_t> GetBinaryResults( const Eigen::Matrix<T,Rows,1>& actuals, const Eigen::Matrix<T,Rows,1>& predictions );
		template<typename T>
		up<std::map<size_t, std::map<T,size_t>>> GetUniqueValues(const Eigen::SparseMatrix<T>& vector );
		template<typename T>
		std::map<std::vector<bool>,std::map<T,size_t>> GetValueCounts( const Eigen::SparseMatrix<T>& sparse );

		// JDE_NATIVE_VISIBILITY Eigen::VectorXd Divide( const Eigen::VectorXd& numerator, double denominator );
		// JDE_NATIVE_VISIBILITY Eigen::MatrixXd Divide( const Eigen::MatrixXd& numerator, const Eigen::VectorXd& denominator );
		// JDE_NATIVE_VISIBILITY Eigen::MatrixXd Divide( double numerator, const Eigen::MatrixXd& denominator );
		// JDE_NATIVE_VISIBILITY Eigen::MatrixXd DotDivide( const Eigen::MatrixXd& numerator, double denominator );
		// JDE_NATIVE_VISIBILITY Eigen::MatrixXd DotPower( double base, const Eigen::MatrixXd& exponent );
		// JDE_NATIVE_VISIBILITY Eigen::MatrixXd DotPower( const Eigen::MatrixXd& base, double exponent );

		template<typename T>
		void DotPowerTransform( SparseMatrix<T>& base, T exponent );

		template<typename T,int Rows,int Cols,int LookupRows>
		up<Eigen::Matrix<T,Rows,Cols>> GetIndexes( const Eigen::Matrix<T,LookupRows,Cols>& lookup, const Eigen::Matrix<size_t,Rows,1>& indexes );
		//template<typename T=float,int Cols=-1>
		//void GetIndexes( const RowVector<float,Cols>& values, const vector<uint>& indexes, RowVector<float,Cols>& results )noexcept;
		template<typename T,int Rows>
		up<Eigen::Matrix<bool, Rows,1>> GetIndexes( const Eigen::Matrix<T,Rows,1>& vector, std::function<bool(T)> function );

		template<typename T>
		up<Eigen::VectorXi> GetColumnCounts( const SparseMatrix<T>& sparse, const std::vector<int>* pColumnIndexes=nullptr );
#pragma region GetRows
		//JDE_NATIVE_VISIBILITY Eigen::MatrixXd GetRows( const Eigen::MatrixXd& matrix, std::vector<size_t>& indexes );
		//inline Eigen::VectorXd GetValues( const Eigen::VectorXd& vector, std::vector<size_t>& indexes );
		template<typename T>
		std::pair<up<Eigen::Matrix<T,-1,1>>,up<Eigen::Matrix<int,-1,1>>> GetValues( const Eigen::SparseMatrix<T>& sparse, const int columnIndex );
#pragma endregion
		template<size_t Rows>
		up<MatrixD<Rows,3>> Hsv();

		template<size_t Rows>
		up<MatrixD<Rows,3>> Hsv2Rgb( MatrixD<Rows,3> hsv );

		//JDE_NATIVE_VISIBILITY std::tuple<MSharedPtr<size_t,-1,-1>,sp<map<size_t,string>>> CategoriesLoad( sv csvFileName, std::vector<string>& columnNamesToFetch, size_t maxLines=std::numeric_limits<size_t>::max(), bool notColumns=false, int chunkSize=100000 );
		//JDE_NATIVE_VISIBILITY std::tuple<MSharedPtr<size_t,-1,-1>,MSharedPtr<size_t,-1,-1>,vector<string>> CategoriesExpand( sv csvTrain, sv csvTest, std::vector<string>& columnNamesToFetch, size_t maxTrainLines=std::numeric_limits<size_t>::max(), size_t maxTestLines=std::numeric_limits<size_t>::max(), bool notColumns=false, int chunkSize=100000 );

		template<typename T=double, int Rows=-1, int Cols=-1>
		α LoadCsv( path csvFileName, vector<string>* pColumnNamesToFetch=nullptr, bool notColumns=false, uint startLine=1, uint maxLines=std::numeric_limits<uint>::max(), T emptyValue=0.0 )ε->MPtr<T,Rows,Cols>;

		// JDE_NATIVE_VISIBILITY Eigen::MatrixXd Log( const Eigen::MatrixXd& exponent );
		// JDE_NATIVE_VISIBILITY Eigen::MatrixXd LogNegative( const Eigen::MatrixXd& exponent );
		// //JDE_NATIVE_VISIBILITY double Norm( const Eigen::VectorXd& vector,  );//
		// JDE_NATIVE_VISIBILITY double Max( const Eigen::VectorXd& matrix );
		// JDE_NATIVE_VISIBILITY Eigen::MatrixXd Max( const Eigen::MatrixXd& matrix, double scaler );
		template<typename T, int Cols>
		std::pair<T,size_t> Max( const Eigen::Matrix<T,1,Cols>& rowVector );
		template<typename T>
		std::tuple<T,int,int> Max( const SparseMatrix<T>& sparse );

		//JDE_NATIVE_VISIBILITY std::pair<Eigen::VectorXd,Eigen::VectorXi> MaxColumn( const Eigen::MatrixXd& matrix );

		template<typename T, int Cols>
		std::pair<T,size_t> Min( const Eigen::Matrix<T,1,Cols>& rowVector );
		template<typename T>
		std::tuple<T,int,int> Min( const SparseMatrix<T>& sparse );
		template<typename T, int Rows=-1>
		up<Eigen::Matrix<T,Rows,1>> MinRowwise( const SparseMatrix<T>& sparse );

		template<typename T,int Rows=-1>
		double Mcc( const Eigen::Matrix<T,Rows,1>& actual, const Eigen::Matrix<T,Rows,1>& predicted );

		template<int RowCount,int ColumnCount>
		up<MatrixD<RowCount,ColumnCount>> Multiply( const MatrixD<RowCount,ColumnCount>& matrix, double scaler );

		// JDE_NATIVE_VISIBILITY Eigen::VectorXd Multiply( const Eigen::VectorXd& vector, double scaler );
		// JDE_NATIVE_VISIBILITY Eigen::MatrixXd DotMultiply( const Eigen::MatrixXd& matrix1, const Eigen::MatrixXd& matrix2 );
		// JDE_NATIVE_VISIBILITY Eigen::MatrixXd Multiply( const Eigen::MatrixXd& matrix, const Eigen::VectorXd& vector );
		// JDE_NATIVE_VISIBILITY Eigen::VectorXd Multiply( const Eigen::VectorXd& v1, const Eigen::VectorXd& v2 );
		// JDE_NATIVE_VISIBILITY std::tuple<Eigen::MatrixXd,Eigen::VectorXd,Eigen::VectorXd> NormalizeOld( const Eigen::MatrixXd& matrix, bool sample=true );
		template<typename T,int Rows,int Cols>
		std::tuple<up<Eigen::Matrix<T,Rows,Cols>>,up<RowVector<T,Cols>>,up<RowVector<T,Cols>>> Normalize( const Eigen::Matrix<T,Rows,Cols>& matrix, bool sample=true );

		template<typename T,int Rows,int Cols>
		Eigen::Matrix<T,Rows,Cols> PseudoInverse( const Eigen::Matrix<T,Rows,Cols>& matrix, Eigen::DecompositionOptions flags=(Eigen::DecompositionOptions)(Eigen::ComputeThinU | Eigen::ComputeThinV) );
		template<int Rows,int Cols>
		up<Eigen::MatrixXd> PolyFeatures( const MatrixD<Rows,Cols>& matrix, size_t count );
		template<typename T,int Rows,int Cols, int ResultCols=Cols+1>
		up<Eigen::Matrix<T,Rows,ResultCols>> Prefix( const Eigen::Matrix<T,Rows,Cols>& matrix, T value );

		// JDE_NATIVE_VISIBILITY up<Eigen::MatrixXd> PrefixTransform( up<Eigen::MatrixXd>, double value );
		// JDE_NATIVE_VISIBILITY Eigen::MatrixXd PrefixOld( const Eigen::MatrixXd& matrix, double value );
		template<int Rows=-1, typename Scaler=size_t>
		up<Eigen::Matrix<Scaler,Rows,1>> RandomIndexes( Scaler upperBound, unsigned seed=0 );

		template<typename T=float,int Rows=-1, int Cols=-1>
		void SetRandomColumn( Matrix<T,Rows,Cols>& matrix, uint excludedColumnIndex, uint randomColumnIndex=std::numeric_limits<uint>::max() )noexcept;

		template<typename T>
		std::tuple<up<Eigen::SparseMatrix<T>>,up<Eigen::Matrix<T,-1,-1>>,std::vector<std::string>> RemoveDuplicates( const Eigen::SparseMatrix<T>& x, const Eigen::Matrix<T,-1,-1>& correlationCoeficient, const size_t offset, std::vector<std::string>& originalColumnNames, const T maxCorrelation );
		template<typename T,int Rows=-1, int Cols=-1>
		void RemoveColumn( Eigen::Matrix<T,Rows,Cols>& matrix, size_t columnIndex );
		template<typename T>
		up<Eigen::SparseMatrix<T>> RemoveColumn( const Eigen::SparseMatrix<T>& sparse, const int columnIndex );

		//JDE_NATIVE_VISIBILITY void RemoveCategoryOrphans( Eigen::Matrix<size_t,-1,-1>& matrix1, Eigen::Matrix<size_t,-1,-1>& matrix2 );

		template<typename T,int Rows=-1, int Cols=-1>
		void RemoveRow( Eigen::Matrix<T,Rows,Cols>& matrix, size_t rowIndex );

		//JDE_NATIVE_VISIBILITY Eigen::MatrixXd Reshape( const Eigen::MatrixXd& matrix, size_t rowCount, size_t columnCount );

		template<typename T, int SortCount, int Rows=-1, int Cols=-1>
		up<Eigen::Matrix<T,Rows,Cols>> Sort(const Eigen::Matrix<T,Rows,Cols>& matrix, const std::array<int,SortCount>& columnIndexes, bool reverse=false, std::vector<size_t>* pOldRowIndexes=nullptr );
		template<typename T, int Rows=-1, int Cols=-1>
		up<Eigen::Matrix<T,Rows,Cols>> Sort(const Eigen::Matrix<T,Rows,Cols>& matrix, const std::vector<size_t>& rowIndexes );

		//JDE_NATIVE_VISIBILITY std::pair<Eigen::VectorXd,Eigen::VectorXd> StandardDeviationColumnsOld( const Eigen::MatrixXd& matrix, bool sample/*=true*/ );
		template<typename T,int Rows,int Cols>
		RowVectorPtr<T,Cols> StandardDeviationColumns( const Eigen::Matrix<T,Rows,Cols>& matrix, bool sample=true, Eigen::Matrix<T,1,Cols>* pAverage=nullptr );
		template<typename T>
		std::pair<up<Eigen::SparseVector<T>>,up<Eigen::SparseVector<T>>> StandardDeviationColumns( const Eigen::SparseMatrix<T>& matrix, bool sample=true );

		// JDE_NATIVE_VISIBILITY double Sum( const Eigen::MatrixXd& matrix );
		// JDE_NATIVE_VISIBILITY double Sum( const Eigen::VectorXd& vector );
		// JDE_NATIVE_VISIBILITY Eigen::VectorXd SumColumns( const Eigen::MatrixXd& matrix, const std::function<double(double)>& function );
		// JDE_NATIVE_VISIBILITY Eigen::VectorXd SumColumnsOld( const Eigen::MatrixXd& matrix );
		template<int Rows,int Cols>
		up<RowVectorD<Cols>> SumColumns( const MatrixD<Rows,Cols>& matrix );
		template<typename T>
		up<Eigen::SparseVector<T>> SumRows( const SparseMatrix<T>& matrix );
		// JDE_NATIVE_VISIBILITY Eigen::VectorXd SumRows( const Eigen::MatrixXd& matrix );
		// JDE_NATIVE_VISIBILITY Eigen::VectorXd LinSpace( double base, double limit, size_t n );

		template<typename T>
		RowVectorPtr<T,-1> ToDenseRowVector( const Eigen::SparseVector<T>& vector );

		//JDE_NATIVE_VISIBILITY Eigen::VectorXd ToVector( const Eigen::MatrixXd matrix1, const Eigen::MatrixXd matrix2 );

		template<typename T,int64_t Rows,int64_t Cols>
		up<Eigen::Matrix<T,-1,Cols>> Select( const Eigen::Matrix<T,Rows,Cols>& matrix, const Eigen::Matrix<bool,Rows,1>& selection, bool notSelection=false );

		template<typename T>
		up<Eigen::SparseMatrix<T>> Select( const Eigen::SparseMatrix<T>& sparse, const int columnIndex, const std::function<bool(int i, const T& value)>& where );
		template<typename T>
		up<Eigen::SparseMatrix<T>> Select( const Eigen::SparseMatrix<T>& sparse, const std::vector<int>& columnIndexes );

		template<typename T>
		std::pair<up<Eigen::SparseVector<T>>,T> TopValue( const Eigen::SparseVector<T>& vector, size_t count );

		template<typename T,int64_t Rows,int64_t Cols>
		void Transform( Eigen::Matrix<T,Rows,Cols>& matrix, const std::function<void(size_t,size_t,T&)>& function );

		template<typename T,int Rows,int Cols>
		RowVectorPtr<T,Cols> VarianceColumns( const Eigen::Matrix<T,Rows,Cols>& matrix, bool sample=true, Eigen::Matrix<T,1,Cols>* pAverage=nullptr );

		template<typename T=float,int64_t Rows=-1,int64_t Cols=-1>
		MPtr<T,Rows,Cols> RemoveRows( const Eigen::Matrix<T,Rows,Cols,0>& matrix, const flat_set<uint>& indexes )noexcept;

		template<typename T=float,int Rows=-1,int Cols=-1,int ResultCols=-1>
		MPtr<T,Rows,ResultCols> ExtractColumns( const Eigen::Matrix<T,Rows,Cols,0>& matrix, const vector<uint>& indexes )noexcept;
		template<typename T>
		up<Matrix<T,1,-1,1>> ExtractColumnsRV( const Eigen::Matrix<T,1,-1>& matrix, const vector<uint>& indexes )noexcept;

		// JDE_NATIVE_VISIBILITY void CompareMatrix( const Eigen::MatrixXd& expected, const Eigen::MatrixXd& actual, double tolerance, sv description, std::function<void(size_t,size_t)> compareLength );
		// JDE_NATIVE_VISIBILITY void CompareVector( const Eigen::VectorXd& expected, const Eigen::VectorXd& actual, double tolerance, sv description, std::function<void(size_t,size_t)> compareLength );
}}
#include "ESparse.h"
#define var const auto

namespace Jde
{
#pragma region ForEach
/*	template<typename T,int64_t Rows,int64_t Columns>
	void EMatrix::ForEachCell( const Matrix<T,Rows,Columns>& matrix, std::function<void(T)> function )
	{
		const auto size = matrix.size();
		if( size==0 )
			return;
		for( uint i = 0; i < size; ++i )
		{
			T temporary = *(matrix.data() + i);
			function( temporary );
		}
	}
*/
	template<typename T,int64_t Rows,int64_t Cols>
	void EMatrix::ForEach( const Matrix<T,Rows,Cols,0>& matrix, const std::function<void(size_t,size_t,T)>& function )
	{
		const uint size = matrix.size();
		const auto /*columnCount = matrix.cols(),*/ rowCount = matrix.rows();
		//const bool isRowMajor = matrix.IsRowMajor;
		for( uint i = 0; i < size; ++i )
		{
			const auto rowIndex = /*isRowMajor ? i/rowCount :*/ i%rowCount;
			const auto columnIndex = /*isRowMajor ? i%rowCount :*/ i/rowCount;
			T value = *(matrix.data() + i);
			function( rowIndex, columnIndex, value );
		}
	}
	template<typename T,int64_t Rows,int64_t Cols>
	void EMatrix::ForEachRef( Matrix<T,Rows,Cols>& matrix, size_t j, const std::function<void(size_t,T&)>& function )
	{
		for( size_t i = 0, size = matrix.rows(); i < size; ++i )
			function( i, matrix.coeffRef(i,j) );
	}

	template<typename T,int64_t Rows>
	void EMatrix::ForEach( const Matrix<T,Rows,1>& vector, const std::function<void(size_t,T)>& function )
	{
		const auto size = vector.size();
		for( int64_t i = 0 ; i < size; ++i )
		{
			//auto rowIndex = isRowMajor ? i/rowCount : i%rowCount;
			T value = *(vector.data() + i);
			function( i, value );
		}
	}
	template<typename T,int64_t Cols>
	void EMatrix::ForEachRowVector( const RowVector<T,Cols>& rowVector, const std::function<void(size_t,T)>& function )
	{
		//const auto rowCount = vector.rows();
		//const auto isRowMajor = vector.IsRowMajor;
		const auto size = rowVector.size();
		for( int64_t i = 0 ; i < size; ++i )
		{
			//auto rowIndex = isRowMajor ? i/rowCount : i%rowCount;
			T value = *(rowVector.data() + i);
			function( i, value );
		}
	}
/*	template<typename T>
	void EMatrix::ForEachValueRef( const SparseMatrix<T>& sparse, const std::function<void(int,int,T&)>& function, Stopwatch* pStopwatch )
	{
		size_t index=0;
		const size_t nonZeros = sparse.nonZeros();
		const size_t progressIndex = std::max( size_t(10000), size_t(nonZeros/10) );
		const size_t columnCount = sparse.outerSize();
		//clog << "ForEachValue::columnCount = " << columnCount <<  std::endl;
		for( auto columnIndex=0; columnIndex<columnCount; ++columnIndex )
		{
			//clog << "\tForEachValue::columnIndex = " << columnIndex <<  std::endl;
			for( typename Eigen::SparseMatrix<T>::InnerIterator it(sparse,columnIndex); it; ++it )
			{
				const auto rowIndex = it.row();
				auto value = it.value();
				function( rowIndex, columnIndex, value );
				if( pStopwatch!=nullptr && ++index%progressIndex==0 )
					pStopwatch->Progress( index, nonZeros );
			}
		}
	}
*/
/*	template<typename T>
	void EMatrix::ForEachValue3( const SparseMatrix<T>& sparse, const std::vector<size_t>* pColumnIndexes, const std::function<void(int,int,T&)>& function )
	{
		if( pColumnIndexes )
		{
			int newColumnIndex = 0;
			for( const auto j : *pColumnIndexes )
			{
				for( Eigen::SparseMatrix<T>::InnerIterator it(sparse,int(j)); it; ++it )
					function( it.row(), newColumnIndex, it.valueRef() );
				++newColumnIndex;
			}

		}
		else
			ForEachValueRef<T>( sparse, function );
	}
*/

/*	template<typename T>
	static void EMatrix::ForEach( const Eigen::SparseVector<T>& vector, const std::function<void(int,T)>& function )
	{
		const auto rowCount = vector.rows();
		for( int rowIndex=0; rowIndex<rowCount; ++rowIndex )
			function( rowIndex, vector.coeff(rowIndex) );
	}
*/

	template<typename T>
	void EMatrix::TransformEachValue( SparseMatrix<T>& sparse, const std::function<void(int,int,T&)>& function )
	{
		for( auto columnIndex=0; columnIndex<sparse.outerSize(); ++columnIndex )
		{
			for( typename Eigen::SparseMatrix<T>::InnerIterator it(sparse,columnIndex); it; ++it )
				function( it.row(), columnIndex, it.valueRef() );
		}
	}
	template<typename T>
	void EMatrix::TransformEachValue( Eigen::SparseVector<T>& sparse, const std::function<void(int,T&)>& function )
	{
		for( typename  Eigen::SparseVector<T>::InnerIterator it(sparse,0); it; ++it )
			function( it.row(), it.valueRef() );
	}
#pragma endregion
#pragma region Assign
	template<typename T,int64_t Rows,int64_t Cols>
	void EMatrix::Assign( Matrix<T,-1,-1>& toMatrix, const Eigen::Matrix<T,Rows,Cols,0>& fromMatrix, uint startingRowIndex )
	{
		auto function = [&toMatrix, startingRowIndex]( uint rowIndex, uint columnIndex, T value )mutable
		{
			toMatrix( startingRowIndex+rowIndex, columnIndex ) = value;
		};
		ForEach<T,Rows,Cols>( fromMatrix, function );
	}
	template<typename T,size_t RowCount,size_t ColumnCount>
	void EMatrix::AssignEachCell( Eigen::Matrix<T,RowCount,ColumnCount,0>& matrix, std::function<T(T)> function )
	{
		if( matrix.size() == 0)
			return;
		for( size_t i = 0, size = matrix.size(); i < size; ++i )
		{
			T temporary = *(matrix.data() + i);
			*(matrix.data() + i) = function(temporary);
		}
	}
	template<typename T,size_t RowCount,size_t ColumnCount>
	void EMatrix::AssignEachCellWithIndex( Eigen::Matrix<T,RowCount,ColumnCount>& matrix, std::function<T(T, size_t)> function )
	{
		if( matrix.size() == 0)
			return;
		for( size_t i = 0, size = matrix.size(); i < size; ++i )
		{
			T temporary = *(matrix.data() + i);
			*(matrix.data() + i) = function(temporary, i);
		}
	}

#pragma endregion
#pragma region Auc
	template<typename T,int Rows>
	std::tuple<double,double,double> EMatrix::Auc( const Eigen::Matrix<T,Rows,1>& actuals, const Eigen::Matrix<T,Rows,1>& predictions )
	{
		const size_t rowCount = actuals.rows();
		assert( rowCount==predictions.rows() );

		std::vector<size_t> oldRowIndexes;
		auto pSortedPredicted = Sort<T,1,Rows,1>( predictions, std::array<int,1>{0}, true, &oldRowIndexes );
		auto pActuals = Sort( actuals, oldRowIndexes );
		const size_t successCount = Count<T,Rows,1>( actuals, T(1.0) );
		const size_t failCount = rowCount-successCount;
		const double successStepSize = 1.0/double(successCount);
		const double failStepSize = 1.0/double(failCount);
		const double aAccuracyLine = double(failCount)/double(successCount);//slope
		double maxAccuracy = 0.0;
		double auc = 0.0;
		double height = 0.0;
		double x = 0.0;
		double cutoff = 0.0;
		for( int i=0; i<rowCount; ++i )
		{
			T actual = pActuals->coeff(i);
			if( actual == 1.0 )
				height += successStepSize;
			else
			{
				x += failStepSize;
				auc += height * failStepSize;
			}
			const double bAccuracyLine = height-x*aAccuracyLine;
			const double accuracy = 1.0-(1.0-bAccuracyLine)/(1+aAccuracyLine);
			if( accuracy>maxAccuracy )
			{
				maxAccuracy = accuracy;
				cutoff = pSortedPredicted->coeff(i);
			}
		}
		return std::make_tuple( cutoff, maxAccuracy, auc );
	}
#pragma endregion
#pragma region AppendColumn
	template<typename T>
	void EMatrix::AppendColumn( Matrix<T,-1,-1,0>& matrix, const Vector<T,-1>& column )
	{
		matrix.conservativeResize( Eigen::NoChange, matrix.cols()+1 );
		matrix.col( matrix.cols()-1 ) = column;
	}

	template<typename T>
	EMatrix::MSharedPtr<T> EMatrix::AppendColumns( const Matrix<T,-1,-1,0>& matrix, const Matrix<T,-1,-1,0>& append )
	{
		const auto startColumn = matrix.cols();
		auto pResults = make_shared<Matrix<T,-1,-1,0>>( matrix.rows(), startColumn+append.cols() );
		auto copy = [&pResults]( uint row, uint column, float value ){ pResults->coeffRef(row, column) = value; };
		ForEach<T,-1,-1>( matrix, copy );
		ForEach<T,-1,-1>( append, [&pResults, startColumn]( uint row, uint column, float value ){ pResults->coeffRef(row, startColumn+column) = value; } );
		return pResults;
	}
#pragma endregion
#pragma region AverageColumns
	template<typename T,int Rows,int Cols>
	Eigen::Matrix<T,1,Cols> EMatrix::AverageColumns( const Eigen::Matrix<T,Rows,Cols>& matrix )
	{
		return matrix.colwise().mean();
	}
#pragma endregion
#pragma region BsxFun
	template<int Rows,int Cols, int Matrix2Rows>
	EMatrix::MPtr<double,Rows,Cols> EMatrix::BsxFun( const M<Rows,Cols>& matrix1, const M<Matrix2Rows,Cols>& matrix2, const std::function<double(double,double)>& binaryFunction )
	{
		M<Rows,Cols>* result = new M<Rows,Cols>();
		const bool isRowVector = matrix2.rows()==1;
		std::function<void(size_t,size_t,double)> cellFunction = [isRowVector,&result,&matrix2, &binaryFunction](size_t rowIndex,size_t columnIndex,double value)mutable
		{
			const size_t matrix2RowIndex = isRowVector ? 0 : rowIndex;
			const double cellValue = binaryFunction( value, matrix2(matrix2RowIndex, columnIndex) );
			(*result)(rowIndex,columnIndex) = cellValue;
		};
		ForEach<double,Rows,Cols>( matrix1, cellFunction );
		return MPtr<double,Rows,Cols>(result);
	}
#pragma endregion
#pragma region BsxFunTransform
	template<int Rows,int Cols>
	void EMatrix::BsxFunTransform( MatrixD<Rows,Cols>& matrix, const RowVectorD<Cols>& rowVector, const std::function<double(double,double)>& binaryFunction )
	{
		std::function<double(size_t,size_t,double)> cellFunction = [&rowVector, &binaryFunction](size_t,size_t columnIndex,double value)
		{
			return binaryFunction( value, rowVector(columnIndex) );
		};
		//*(matrix.data() + i) = function( rowIndex, columnIndex, value );
		Transform<double,Rows,Cols>( matrix, cellFunction );
	}
#pragma endregion
#pragma region Centered
	template<typename T, int Rows, int Cols>
	Eigen::Matrix<T,Rows,Cols> EMatrix::Centered( const Eigen::Matrix<T,Rows,Cols>& matrix, Eigen::Matrix<T,1,Cols>* pAverage/*=nullptr*/ )
	{
		//Eigen::Matrix<T,1,Cols> average = pAverage==nullptr ? AverageColumns(matrix) : *pAverage;
		//auto result =
		return matrix.rowwise() - (pAverage==nullptr ? AverageColumns(matrix) : *pAverage);
	}
#pragma endregion
#pragma region CorrelationCoefficient
	template<typename T>
	up<Eigen::Matrix<T,-1,-1>> EMatrix::CorrelationCoefficient( const SparseMatrix<T>& sparse, Eigen::Matrix<T,-1,-1>* pExisting, size_t threadCount/*=1*/, Stopwatch* pStopwatch/*=nullptr*/, size_t columnCount/*=std::numeric_limits<size_t>::max()*/, const std::function<void(const Eigen::Matrix<float,-1,-1>&)>* saveFunction/*=nullptr*/ )
	{
		return Covariance( sparse, pExisting, true, threadCount, pStopwatch, columnCount, saveFunction );
	}
#pragma endregion
#pragma region Covariance
	template<typename T, int Rows, int Cols>
	Eigen::Matrix<T,Cols,Cols> EMatrix::Covariance( const Eigen::Matrix<T,Rows,Cols>& matrix, Eigen::Matrix<T,1,Cols>* pAverage/*=nullptr*/ )
	{
		auto centered = Centered( matrix, pAverage );
		return (centered.adjoint() * centered) / T(matrix.rows() - 1);
	}

	template<typename T>
	up<Eigen::Matrix<T,-1,-1>> EMatrix::Covariance( const SparseMatrix<T>& sparse, Eigen::Matrix<T,-1,-1>* pExisting, bool correlationCoefficient/*=false*/, size_t threadCount/*=1*/, Stopwatch* pStopwatch/*=nullptr*/, size_t maxColumnCount/*=std::numeric_limits<size_t>::max()*/, const std::function<void(const Eigen::Matrix<float,-1,-1>&)>* pSaveFunction/*=nullptr*/ )
	{
		const size_t columnCount = std::min<size_t>( maxColumnCount, sparse.cols() );
		//clog << "columnCount = " << columnCount << "  maxColumnCount=" << maxColumnCount << "  sparse.cols()=" << sparse.cols();
		auto pResult = pExisting ? pExisting : new Eigen::Matrix<T,-1,-1>( sparse.cols(), sparse.cols() );
		size_t iterationCount = size_t(std::pow<size_t>(columnCount,2)/2)-1;
		//const size_t progressIndex = std::max( size_t(10000), size_t(iterationCount/10) );
		std::atomic<size_t> iteration = 0;
		//constexpr zeroValue = std::make_pair( T(0), T(0) );
		bool allocateStopwatch = pStopwatch==nullptr;
		if( allocateStopwatch )
			pStopwatch = new Stopwatch( fmt::format("StopwatchTypes::Calculate - {}", "Covariance"), false );
		//const static std::chrono::duration<size_t, std::chrono::nanoseconds::period> saveTimespan( 5*Chrono::TimeSpan::NanosPerMinute );
		constexpr std::chrono::minutes saveTimespan = 5min;
		auto lastSave = pStopwatch->Elapsed();
		std::mutex resultMutex;
		//int threadWaitCount;
		function<void(size_t)> columnFunction = [&sparse, saveTimespan, correlationCoefficient, &iteration, &iterationCount, &pStopwatch,&pResult,&resultMutex, &pSaveFunction, &lastSave, &threadCount]( size_t columnIndex1 )mutable
		{
			for( int columnIndex2=0; columnIndex2<sparse.cols(); ++columnIndex2 )
			{
				if( columnIndex2<=columnIndex1 )
				{
					if( columnIndex1==columnIndex2 )
					{
						//resultMutex.lock();
						pResult->coeffRef(columnIndex1,columnIndex2)=T(0);
						//resultMutex.unlock();
					}
					continue;
				}
				std::set<int> perfectCorrelations;
				//for( int iPreviousColumnIndex=0; iPreviousColumnIndex< )
				if( pResult->coeffRef( columnIndex1, columnIndex2 )!=0 &&  pResult->coeffRef( columnIndex2, columnIndex1 )!=0 )
				{
					--iterationCount;
					continue;
				}
				//Stopwatch loop1( pStopwatch, StopwatchTypes::Calculate, "loop1" );
				int rowIndex1 = -1;
				int rowIndex2 = -1;
				std::unordered_map<int,std::pair<T,T>> values;
				auto pInsertHint = values.begin();

				for( typename Eigen::SparseMatrix<T>::InnerIterator it(sparse,int(columnIndex1)), it2(sparse,columnIndex2); it || it2; )
				{
					rowIndex1 = it ? it.row() : std::numeric_limits<int>::max();
					rowIndex2 = it2 ? it2.row() : std::numeric_limits<int>::max();
					if( rowIndex1==rowIndex2 )
					{
						pInsertHint = values.emplace_hint( pInsertHint, rowIndex1, std::make_pair(it.value(), it2.value()) );
						++it;
						++it2;
					}
					else if( rowIndex1<rowIndex2 )
					{
						pInsertHint = values.emplace_hint( pInsertHint, std::make_pair(rowIndex1, std::make_pair(it.value(), T(0))) );
						++it;
					}
					else if( rowIndex2<rowIndex1 )
					{
						pInsertHint = values.emplace_hint( pInsertHint, std::make_pair(rowIndex2, std::make_pair(T(0), it2.value())) );
						++it2;
					}
				}
				//loop1.Finish();
				//Stopwatch loop2( pStopwatch, StopwatchTypes::Calculate, "loop2" );
				const size_t rowCount = values.size();
				constexpr int Rows=-1;
				constexpr int Cols=2;
				Eigen::Matrix<T,Rows,Cols> matrix( rowCount, 2 );
				int rowIndex=0;
				for( const auto& value : values )
				{
					matrix(rowIndex, 0) = value.second.first;
					matrix(rowIndex++, 1) = value.second.second;
				}
				T divisor = T(1);
				//loop2.Finish();
				Eigen::Matrix<T,1,Cols> average;
				if( correlationCoefficient )
				{
					//Stopwatch sw4( pStopwatch, StopwatchTypes::Calculate, "Average" );
					average = AverageColumns( matrix ); //sw4.Finish();
					//Stopwatch sw2( pStopwatch, StopwatchTypes::Calculate, "StandardDeviation" );
					auto pStandardDeviation = StandardDeviationColumns<T,Rows,Cols>( matrix, true/*sample*/, &average ); //sw2.Finish();
					divisor = pStandardDeviation->coeff(0,0)*pStandardDeviation->coeff(0,1);
				}
				//Stopwatch sw3( pStopwatch, StopwatchTypes::Calculate, "Covariance" );
				auto cov = Covariance<T,Rows,Cols>( matrix, correlationCoefficient ? &average : nullptr );//sw3.Finish();

				//resultMutex.lock();
				pResult->coeffRef( columnIndex1, columnIndex2 ) = cov.coeff(0,1)/divisor;
				pResult->coeffRef( columnIndex2, columnIndex1 ) = cov.coeff(1,0)/divisor;//s/b szme as (0,1)
				//resultMutex.unlock();

				++iteration;
				//if( columnIndex2%100==0 )
					//clog << "correlation[  " << columnIndex1 << ", " << columnIndex2 << "]=" << pResult->coeffRef( columnIndex1, columnIndex2 ) << endl;
				//pStopwatch->Progress( iteration, iterationCount );
				if( pSaveFunction!=nullptr && (pStopwatch->Elapsed()-lastSave)>saveTimespan )
				{
					resultMutex.lock();
					if( (pStopwatch->Elapsed()-lastSave)>saveTimespan*threadCount )
					{
						(*pSaveFunction)(*pResult);
						lastSave = pStopwatch->Elapsed();
					}
					resultMutex.unlock();
				}
			}
			//if( pStopwatch!=nullptr /*&& iteration%progressIndex==0*/ )
				pStopwatch->Progress( iteration, iterationCount );
		};
		Threading::Run( threadCount, columnCount, columnFunction );

		if( allocateStopwatch )
			delete pStopwatch;

		return up<Eigen::Matrix<T,-1,-1>>( pExisting ? nullptr : pResult );
	}
#pragma endregion
#pragma region ExportCsv
	template<typename T, int Rows, int Cols>
	void EMatrix::ExportCsv( sv pszFileName, const Eigen::Matrix<T,Rows,Cols>& matrix, const vector<string>* pColumnNames )
	{
		std::ofstream os;
		os.open( string(pszFileName) ); THROW_IF( os.bad(), "Could not open file '{}'", pszFileName );
		if( pColumnNames )
		{
			for( uint i=0; i<pColumnNames->size(); ++i )
			{
				if( i>0 )
					os << ",";
				os << (*pColumnNames)[i];
			}
			os << endl;
		}
		const size_t rowCount = matrix.rows();
		for( uint i=0; i<rowCount; ++i )
		{
			for( int j=0; j<matrix.cols(); ++j )
			{
				if( j>0 )
					os << ",";
				os <<  matrix( i, j );
			}
			os << std::endl;
		}
		os.close();
	}
#pragma endregion
#pragma region ExpTransform
	template<typename T, int64_t Rows, int64_t Cols>
	Eigen::Matrix<T,Rows,Cols>& EMatrix::ExpTransform( Eigen::Matrix<T,Rows,Cols>& matrix )
	{
		auto function = [](size_t, size_t, T& value)mutable{ value = std::exp(value); };
		Transform<T,Rows,Cols>( matrix, function );
		return matrix;
	}

	template<typename T>
	SparseMatrix<T>& EMatrix::ExpTransform( SparseMatrix<T>& matrix )
	{
		auto function = [](int, int, T& value)mutable{ value = std::exp(value); };
		TransformEachValue<T>( matrix, function );
		return matrix;
	}
#pragma endregion
#pragma region Count
	template<typename Type>
	size_t EMatrix::EqualCount( const Eigen::Matrix<Type, Eigen::Dynamic, 1>& v1, const Eigen::Matrix<Type, Eigen::Dynamic, 1>& v2 )
	{
		size_t count = 0;
		std::function<void(size_t,size_t,Type)> countFunction = [&](size_t rowIndex,size_t,double value)mutable{if( v2(rowIndex)==value )++count; };
		ForEach( v1, countFunction );
		return count;
	}
	template<typename T>
	size_t EMatrix::EqualCount( const Eigen::SparseVector<T>& v1, const Eigen::SparseVector<T>& v2 )
	{
		size_t count=0;
		auto countFunction = [&count,&v2](int rowIndex,T value)mutable{if( v2.coeff(rowIndex)==value )++count; };
		ForEach<T>( v1, countFunction );
		return count;
	}

	template<typename T,int Rows, int Cols>
	size_t EMatrix::Count( const Eigen::Matrix<T,Rows, Cols>& matrix, const T value )
	{
		size_t count = 0;
		ForEach<T>( matrix, [&value, &count](size_t, size_t, T rowValue){ if( value==rowValue )++count;} );
		return count;
	}

	template<typename T>
	size_t EMatrix::Count( const Eigen::SparseVector<T>& vector, T value )
	{
		size_t count = 0;
		ESparse::ForEach<T>( vector, [value, &count](int rowIndex, const T* rowValue){ if( (rowValue==nullptr && value==0) ||  (rowValue!=nullptr && value==*rowValue) )++count;} );

		return count;
	}
#pragma endregion
#pragma region GetBinaryResults
	template<typename T, int Rows>
	std::tuple<int64_t,int64_t,int64_t,int64_t> EMatrix::GetBinaryResults( const Eigen::Matrix<T,Rows,1>& actuals, const Eigen::Matrix<T,Rows,1>& predictions )
	{
		assert( actuals.rows()==predictions.rows() );
		int64_t truePositives=0, falsePositives=0, trueNegatives=0, falseNegatives=0;
		for( int i=0; i<actuals.rows(); ++i )
		{
			const bool actual = actuals(i)!=T(0);
			const bool predicted = predictions(i)!=T(0);
			if( actual && predicted )
				++truePositives;
			else if( actual && !predicted )
				++falseNegatives;
			else if( !actual && predicted )
				++falsePositives;
			else if( !actual && !predicted )
				++trueNegatives;
		}
		return std::make_tuple( truePositives, falsePositives, trueNegatives, falseNegatives );
	}
#pragma endregion
#pragma region GetUniqueValues
	///[j][value][count]
	template<typename T>
	up<std::map<size_t, std::map<T,size_t>>> EMatrix::GetUniqueValues(const Eigen::SparseMatrix<T>& matrix )
	{
		Stopwatch sw( "CountUniqueColumns" );
		std::map<size_t, std::map<T,size_t>>* pUniqueValues = new std::map<size_t, std::map<T,size_t>>();
		const size_t columnCount = matrix.outerSize();
		for( int columnIndex = 0; columnIndex<columnCount; ++columnIndex )
			pUniqueValues->insert( make_pair(columnIndex, std::map<T,size_t>()) );
		auto function = [&pUniqueValues](int, int columnIndex, T value)
		{
			//if( columnIndex>2140 )
			//	clog << "[" << columnIndex << "]=" << value << std::endl;
			std::map<T,size_t>& columnMap = (*pUniqueValues)[columnIndex];
			auto& pExistingValue = columnMap.find( value );
			if( pExistingValue==columnMap.end() )
				columnMap.insert( make_pair(value,1) );
			else
				++pExistingValue->second;
		};
		ESparse::ForEachValue<T>(matrix, function, nullptr, &sw);
		//clog << "columnCount = " << columnCount <<  std::endl;
		//auto pResult = new Eigen::VectorXi( columnCount );
		//for( const auto& columnValues : (*pUniqueValues) )
		//{
		//	//clog << "columnIndex[" << columnValues.first << "] = " << columnValues.second.size() << std::endl;
		//	pResult->coeffRef(columnValues.first) = int(columnValues.second.size());
		//}
		auto result = up<std::map<size_t, std::map<T,size_t>>>( pUniqueValues );
		return result;
	}

#pragma endregion
#pragma region GetValueCounts
	template<typename T>
	std::map<std::vector<bool>,std::map<T,size_t>> EMatrix::GetValueCounts( const Eigen::SparseMatrix<T>& sparse )
	{
		const auto columnCount = sparse.cols()-1;
		std::vector<std::pair<std::vector<bool>,T>> values( sparse.rows(), make_pair(std::vector<bool>(columnCount,false),T(0)) );
		const auto responseIndex = columnCount-1;
		auto function = [&values, &columnCount,&responseIndex](int i, int j, T value)
		{
			if( j == responseIndex )
				values[i].second = value;
			else
				values[i].first[j] = true;
		};
		ESparse::ForEachValue<T>( sparse, function );

		std::map<std::vector<bool>,std::map<T,size_t>> results;
		for( const auto& rowResult : values )
		{
			const auto& row = rowResult.first;
			const T& result = rowResult.second;

			auto pExisting = results.find( row );
			if( pExisting==results.end() )
			{
				std::map<T,size_t> resultCounts;
				resultCounts.insert( make_pair(result, 1) );
				results.insert( make_pair(row, resultCounts) );
			}
			else
				++pExisting->second[result];
		}

		return results;
	}
#pragma endregion
#pragma region DotPowerTransform
	template<typename T>
	void EMatrix::DotPowerTransform( SparseMatrix<T>& base, T exponent )
	{
		MatrixXd result( base.rows(), base.cols() );
		std::function<void(int,int,T&)> powerFunction = [&exponent](int,int,T& value)mutable{value = std::pow(value,exponent); };
		EMatrix::TransformEachValue( base, powerFunction );
	}
#pragma endregion
#pragma region GetIndexes
	template<typename T,int Rows>
	up<Eigen::Matrix<bool, Rows,1>> EMatrix::GetIndexes( const Eigen::Matrix<T,Rows,1>& vector, std::function<bool(T)> function )
	{
		const auto rowCount = vector.rows();
		Eigen::Matrix<bool, Rows,1>* pIndexes = new Eigen::Matrix<bool, Rows,1>( rowCount );
		auto indexFunction = [pIndexes,function](size_t rowIndex,T value)mutable{(*pIndexes)(rowIndex) = function(value); };
		ForEach<T,Rows>( vector, indexFunction );
		return up<Eigen::Matrix<bool, Rows,1>>(pIndexes);
	}

	template<typename T,int Rows,int Cols,int LookupRows>
	up<Eigen::Matrix<T,Rows,Cols>> EMatrix::GetIndexes( const Eigen::Matrix<T,LookupRows,Cols>& lookup, const Eigen::Matrix<size_t,Rows,1>& indexes )
	{
		Eigen::Matrix<T,Rows,Cols>* result = new Eigen::Matrix<T,Rows,Cols>( indexes.rows(), lookup.cols() );
		std::function<void(size_t ,size_t)> function = [&result, lookup](size_t rowIndex,size_t value)mutable{result->row(rowIndex)=lookup.row(value); };
		EMatrix::ForEach<size_t,Rows> ( indexes, function );
		return up<Eigen::Matrix<T,Rows,Cols>>(result);
	}
#pragma endregion
#pragma region GetColumnCounts
	template<typename T>
	up<Eigen::VectorXi> EMatrix::GetColumnCounts( const SparseMatrix<T>& sparse, const std::vector<int>* pColumnIndexes )
	{
		const auto columnCount = pColumnIndexes ? pColumnIndexes->size() : sparse.cols();
		Eigen::VectorXi* pColumnCounts = new Eigen::VectorXi( Eigen::VectorXi::Zero(columnCount) );
		if( sparse.innerNonZeroPtr()!=nullptr )
		{
			if( pColumnIndexes )
			{
				int j=0;
				for( const auto columnIndex : *pColumnIndexes )
					pColumnCounts->coeffRef(j++) = sparse.innerNonZeroPtr()[columnIndex];
			}
			else
			{
				for( int columnIndex = 0; columnIndex<columnCount; ++columnIndex )
					pColumnCounts->coeffRef(columnIndex) = sparse.innerNonZeroPtr()[columnIndex];
			}
		}
		else
		{
			auto function = [pColumnCounts](int, int j, T ){ pColumnCounts->coeffRef(j)++; };
			ESparse::ForEachValue<T>( sparse, function, pColumnIndexes );
		}
		return up<Eigen::VectorXi>(pColumnCounts);
	}
#pragma endregion
#pragma region GetValues
/*	Eigen::VectorXd EMatrix::GetValues( const Eigen::VectorXd& vector, std::vector<size_t>& indexes )
	{
		Eigen::VectorXd values( indexes.size() );
		size_t size = indexes.size();
		for( uint iRow=0; iRow<size; ++iRow )
			values(iRow) = vector( indexes[iRow] );

		return values;
	}
*/
	template<typename T>
	std::pair<up<Eigen::Matrix<T,-1,1>>,up<Eigen::Matrix<int,-1,1>>> EMatrix::GetValues( const Eigen::SparseMatrix<T>& sparse, const int columnIndex )
	{
		std::map<int,T> values;

		for( typename Eigen::SparseMatrix<T>::InnerIterator it(sparse,columnIndex); it; ++it )
		{
			const auto rowIndex = it.row();
			const T value = it.value();
			values.insert( make_pair(rowIndex, value) );
		}

		auto pValues = new Eigen::Matrix<T,-1,1>( values.size(),1 );
		auto pIndexes = new Eigen::Matrix<int,-1,1>( values.size(),1 );
		int rowIndex = 0;
		for( const auto& indexValue : values )
		{
			pIndexes->coeffRef(rowIndex,0) = indexValue.first;
			pValues->coeffRef(rowIndex++,0) = indexValue.second;
		}
		return std::make_pair( up<Eigen::Matrix<T,-1,1>>(pValues),up<Eigen::Matrix<int,-1,1>>(pIndexes) );
	}
#pragma endregion
#pragma region Hsv
	template<size_t Rows>
	up<EMatrix::MatrixD<Rows,3>> EMatrix::Hsv()
	{
		up<MatrixD<Rows,3>> result;
		if( Rows==1 )
		{
			result = up<MatrixD<Rows,3>>( new MatrixD<Rows,3>() );
			*result << 1,0,0;
		}
		else
		{
			VectorD<Rows> h = VectorD<Rows>::LinSpaced( Rows,0,double(Rows-1)/Rows ); //h = linspace (0, 1, n)';
			//std::clog << h.array().sum() << std::endl;
			MatrixD<Rows,3> hsv;
			hsv << h, VectorD<Rows>::Ones(), VectorD<Rows>::Ones();
			result =  Hsv2Rgb<Rows>(hsv);//hsv2rgb ([h, ones(n, 1), ones(n, 1)]);

		}
		return std::move( result );
	}
	template<size_t Rows>
	up<EMatrix::MatrixD<Rows,3>> EMatrix::Hsv2Rgb( MatrixD<Rows,3> hsv )
	{
		VectorD<Rows> h = hsv.col(0);
		VectorD<Rows> s = hsv.col(1);
		VectorD<Rows> v = hsv.col(2);

		//Prefill rgb map with v*(1-s)
		VectorD<Rows> zeros = (-s).array()+1;
		MatrixD<Rows,3> rgbMapStart = v.cwiseProduct( zeros ).replicate(1,3);	//Matrix<Rows,3>::Zero(); rgb_map = repmat (v .* (1 - s), 1, 3);
		// red = hue-2/3 : green = hue : blue = hue-1/3
		// ## Apply modulo 1 for red and blue to keep within range [0, 1]

		VectorD<Rows> hue1; hue1 << h.unaryExpr( [](const double x) { double value=x-2.0/3.0; return /*value<0 ? 1.0+value :*/ std::fmod(value+1.0,1.0); } );
		VectorD<Rows> hue2; hue2 << h.unaryExpr( [](const double x) { double value=x-1.0/3.0; return /*value<0 ? 1.0+value :*/ std::fmod(value+1.0,1.0); } );
		MatrixD<Rows,3> hue(Rows,3);
		hue << hue1, h, hue2;// hue = [mod(h - 2/3, 1), h , mod(h - 1/3, 1)];
		//std::clog << hue.array().sum() << std::endl;
		VectorD<Rows> sv = s.array()*v.array();
		MatrixD<Rows,3> f = v.cwiseProduct( sv ).replicate(1,3); //Matrix<Rows,3>::One();  //factor s*v -> f   f = repmat (s .* v, 1, 3);
		//add s*v*hue-function to rgb map
		//rgb_map += f .* (6 * (hue < 1/6) .* hue + (hue >= 1/6 & hue < 1/2) + (hue >= 1/2 & hue < 2/3) .* (4 - 6 * hue));
		MatrixD<Rows,3>* rgbMap = new MatrixD<Rows,3>();
		function<void(size_t,size_t,double)> function = [rgbMap, &f](size_t rowIndex,size_t columnIndex,double hue)
		{
			double value;
			if( hue < 1.0/6.0 )
				value = 6.0*hue;
			else if( hue >= 1.0/6.0 && hue < .5 )
				value = 1.0;
			else if( hue >= .5 && hue < 2.0/3.0 )
				value = 4.0-6.0*hue;
			else
				value = 0.0;
			(*rgbMap)(rowIndex,columnIndex) = f(rowIndex,columnIndex)*value;
		};
		ForEach(hue, function);
		return up<MatrixD<Rows,3>>(rgbMap);
		//98
			//99   ## FIXME: hsv2rgb does not preserve class of image.
			//100   ##        Should it also convert back to uint8, uint16 for integer images?
			//101   ## If input was an image, convert it back into one.
			//102   if (is_image)
			//103     rgb_map = reshape (rgb_map, sz);
		//104   endif
	}
#pragma endregion
#pragma region LoadCsv
	template<typename T, int Rows, int Cols>
	α EMatrix::LoadCsv( path csvFileName, vector<string>* pColumnNamesToFetch, bool notColumns, uint startLine, uint maxLines, T emptyValue )ε->MPtr<T,Rows,Cols>
	{
		Stopwatch sw( csvFileName.string(), true );

		const bool allColumns = pColumnNamesToFetch==nullptr || pColumnNamesToFetch->size()==0;
		vector<string> columnNames;
		flat_set<size_t> columnIndexes;
		if( !allColumns )
		{
			auto namesIndexes = IO::LoadColumnNames( csvFileName, *pColumnNamesToFetch, notColumns ); columnNames=get<0>( namesIndexes ); columnIndexes = get<1>( namesIndexes );
		}
		vector<vector<T>> columnValues;
		auto columnCount = columnNames.size();

		auto getValues = [&sw,&columnValues,&columnCount]( const vector<double>& tokens, size_t lineIndex )
		{
			if constexpr( std::is_same<T, double>::value )
				columnValues.push_back( tokens );
			else
			{
				std::vector<T> v; v.reserve( tokens.size() );
				std::for_each( tokens.begin(), tokens.end(), [&v]( var t ){v.push_back(static_cast<T>(t));} );
				columnValues.push_back( v );
			}
			if( lineIndex%100000==0 )
				sw.Progress( lineIndex );
			if( columnCount==0 )
				columnCount = tokens.size();
			THROW_IF( tokens.size() != columnCount, "Column counts don't add up for line '{}' actual:  '{}' expected:  '{}'", lineIndex, tokens.size(), columnCount );
		};
		IO::ForEachLine( csvFileName, getValues, columnIndexes, maxLines, startLine, 1073741824, 1500, &sw, emptyValue );

		auto y = mu<Matrix<T,Rows,Cols>>( columnValues.size(), columnCount );
		for( uint i=0; i<columnValues.size(); ++i )
		{
			for( uint j=0; j<columnCount; ++j )
				y->coeffRef( i, j ) = T{ columnValues[i][j] };
		}
		return y;
	}

	//auto getValues = /*[&]*/[&columnCounts,&convertFunction, &sw2, &lineCount,&allColumns,&columnIndexes, &pResult](const std::vector<std::string>& tokens, size_t lineIndex)mutable
	//{
	//	//Stopwatch swGetStats( &sw2, StopwatchTypes::Calculate, "getValues", false );
	//	size_t resultColumnIndex=0;
	//	for( auto pToken = tokens.begin(); pToken!=tokens.end(); ++pToken )
	//	{
	//		if( pToken->empty() )
	//			continue;
	//		Stopwatch swToken( &sw2, StopwatchTypes::Calculate, "swToken", false );
	//		swToken.Finish();
	//		Stopwatch swConvert( &sw2, StopwatchTypes::Calculate, "convert", false );
	//		T value = convertFunction(*pToken);
	//		swConvert.Finish();
	//		Stopwatch coeffRef( &sw2, StopwatchTypes::Calculate, "pResult->coeffRef", false );
	//		pResult->coeffRef( int(lineIndex-1), int(resultColumnIndex) ) = value;
	//		coeffRef.Finish();
	//		++resultColumnIndex;
	//	}
	//	if( lineIndex%10000==0 )
	//		sw2.Progress( lineIndex, lineCount );
	//};
	//IO::File::ForEachLine2( csvFileName.c_str(), getValues, columnIndexes, maxLines, 1, 1073741824,  &sw2 );

#pragma endregion
#pragma region Min/Max
	template<typename T, int Cols>
	std::pair<T,size_t> EMatrix::Max( const Eigen::Matrix<T,1,Cols>& rowVector )
	{
		T maximum = std::numeric_limits<T>::min();
		const auto columnCount = rowVector.cols();
		size_t index=0;
		for( int64_t columnIndex=0; columnIndex<columnCount; ++columnIndex )
		{
			const T value = rowVector(0,columnIndex);
			if( value>maximum )
			{
				maximum = value;
				index = columnIndex;
			}
		}
		return std::make_pair( maximum, index );
	}

	template<typename T, int Cols>
	std::pair<T,size_t> EMatrix::Min( const Eigen::Matrix<T,1,Cols>& rowVector )
	{
		T minimum = std::numeric_limits<T>::max();
		const auto columnCount = rowVector.cols();
		size_t index=0;
		for( int64_t columnIndex=0; columnIndex<columnCount; ++columnIndex )
		{
			const T value = rowVector(0,columnIndex);
			if( value<minimum )
			{
				minimum = value;
				index = columnIndex;
			}
		}
		return std::make_pair( minimum, index );
	}
	template<typename T>
	std::tuple<T,int,int> EMatrix::Min( const SparseMatrix<T>& sparse )
	{
		T minimum = std::numeric_limits<T>::max();
		int minRowIndex=-1, minColumnIndex=-1;
		std::function<void(int,int,T)>  minFunction = [&minimum, &minRowIndex, &minColumnIndex](int rowIndex, int columnIndex, T value)mutable
		{
			if( minimum>value )
			{
				minimum = value;
				minRowIndex = rowIndex;
				minColumnIndex = columnIndex;
			}
		};
		ESparse::ForEachValue<T>( sparse, minFunction );
		return std::make_tuple( minimum, minRowIndex, minColumnIndex );
	}
	template<typename T>
	std::tuple<T,int,int> EMatrix::Max( const SparseMatrix<T>& sparse )
	{
		T maximum = -std::numeric_limits<T>::max();
		int maxRowIndex=-1, maxColumnIndex=-1;
		std::function<void(int,int,T)> maxFunction = [&maximum, &maxRowIndex, &maxColumnIndex](int rowIndex, int columnIndex, T value)mutable
		{
			//clog << "value["<< typeid(value).name()  <<"]="<< value << ".  maximum[" << typeid(maximum).name() << "]=" << maximum << std::endl;
			if( maximum<value )
			{
				maximum = value;
				maxRowIndex = rowIndex;
				maxColumnIndex = columnIndex;
			}
		};
		ESparse::ForEachValue<T>( sparse, maxFunction );
		return std::make_tuple( maximum, maxRowIndex, maxColumnIndex );
	}
	template<typename T, int Rows>
	up<Eigen::Matrix<T,Rows,1>> EMatrix::MinRowwise( const SparseMatrix<T>& sparse )
	{
		//auto foo = Eigen::Matrix<T,Rows,1>::Zero(1,1);
		//Eigen::VectorXi::Zero(columnCount)
		auto pResult = new Eigen::Matrix<T,Rows,1>( Eigen::Matrix<T,Rows,1>::Constant( sparse.rows(), 1, std::numeric_limits<T>::max()) );
		auto function = [&pResult]( int i, int j, T value )mutable
		{
			if( value!=0 && value<pResult->coeff(i,0) )
			{
				assert( value!=std::numeric_limits<T>::min() );
				pResult->coeffRef(i,0) = value;
			}
		};
		ESparse::ForEachValue<T>( sparse, function );
		return up<Eigen::Matrix<T,Rows,1>>(pResult);//
	}
#pragma endregion
#pragma region Mcc
	template<typename T,int Rows>
	double EMatrix::Mcc( const Eigen::Matrix<T,Rows,1>& actuals, const Eigen::Matrix<T,Rows,1>& predictions )
	{
		auto br = GetBinaryResults( actuals, predictions );
		int64_t truePositives=std::get<0>(br), falsePositives=std::get<1>(br), trueNegatives=std::get<2>(br), falseNegatives=std::get<3>(br);
		return (truePositives*trueNegatives-falsePositives*falseNegatives)/std::sqrt( (truePositives+falsePositives)*(truePositives+falseNegatives)*(trueNegatives+falsePositives)*(trueNegatives+falseNegatives) );
	}
#pragma endregion
#pragma region Multiply
	template<int RowCount,int ColumnCount>
	up<EMatrix::MatrixD<RowCount,ColumnCount>> EMatrix::Multiply( const MatrixD<RowCount,ColumnCount>& matrix, double scaler )
	{
		MatrixD<RowCount,ColumnCount>* result = new MatrixD<RowCount,ColumnCount>( matrix.rows(), matrix.cols() );
		std::function<void(size_t,size_t,double)> multiplyScalerFunction = [result, scaler](size_t rowIndex,size_t columnIndex,double value)mutable{(*result)(rowIndex,columnIndex)=value*scaler; };
		EMatrix::ForEach( matrix, multiplyScalerFunction );
		return up<MatrixD<RowCount,ColumnCount>>(result);
	}
#pragma endregion
#pragma region Normalize
	template<typename T,int Rows,int Cols>
	std::tuple<up<Eigen::Matrix<T,Rows,Cols>>,up<EMatrix::RowVector<T,Cols>>,up<EMatrix::RowVector<T,Cols>>> EMatrix::Normalize( const Eigen::Matrix<T,Rows,Cols>& matrix, bool sample/*=true*/ )
	{
		RowVector<T,Cols> average = AverageColumns(matrix);
		up<RowVector<T,Cols>> standardDeviation = StandardDeviationColumns<T,Rows,Cols>( matrix, sample, &average );

		auto normalization = new Eigen::Matrix<T,Rows,Cols>( matrix.rows(), matrix.cols() );
		//(*normalization)(1, 1)=0;
		const std::function<void(size_t,size_t,T)> function = [normalization, &average, &standardDeviation ](size_t rowIndex, size_t columnIndex, T value)mutable{(*normalization)(rowIndex, columnIndex)=(value-average(columnIndex))/(*standardDeviation)(columnIndex);};
		ForEach<T,Rows,Cols>( matrix, function );

		return std::make_tuple( up<Eigen::Matrix<T,Rows,Cols>>(normalization), up<RowVector<T,Cols>>(new RowVector<T,Cols>(average)), std::move(standardDeviation) );
	}
#pragma endregion
#pragma region PseudoInverse
	template<typename T,int Rows,int Cols>
	Eigen::Matrix<T,Rows,Cols> EMatrix::PseudoInverse( const Eigen::Matrix<T,Rows,Cols>& matrix, Eigen::DecompositionOptions flags )
	{
		constexpr double pinvtoler = 1.e-6; // choose your tolerance wisely!
		auto svd = Eigen::JacobiSVD<Eigen::Matrix<T,Rows,Cols>>( matrix, flags );
		const Eigen::Matrix<T,Cols,1>& singularValues=svd.singularValues();
		Eigen::Matrix<T,Cols,1> singularValues_inv=svd.singularValues();
		for( long columnIndex=0; columnIndex<matrix.cols(); ++columnIndex )
			singularValues_inv(columnIndex) = singularValues(columnIndex) > pinvtoler ? T(1.0)/singularValues(columnIndex) : T(0.0);

		auto result = svd.matrixV()*singularValues_inv.asDiagonal()*svd.matrixU().transpose();
		//std::clog << "Rows:  " << Rows << endl;
		return Eigen::Matrix<T,Rows,Cols>(result);
	}
#pragma endregion
#pragma region PolyFeatures
	template<int Rows,int Cols>
	up<Eigen::MatrixXd> EMatrix::PolyFeatures( const MatrixD<Rows,Cols>& matrix, size_t count )
	{
		Eigen::MatrixXd* result = new Eigen::MatrixXd( matrix.rows(), matrix.cols()*count );

		auto function = [result, count](size_t rowIndex,size_t columnIndex,double value)mutable
		{
			for( size_t powerIndex=0; powerIndex<count;++powerIndex )
				(*result)( rowIndex, columnIndex*count+powerIndex ) = std::pow( value, double(powerIndex+1) );
		};

		ForEach<double,Rows,Cols>( matrix, function );
		//ones(m,1)*[(1:p)]
		return up<Eigen::MatrixXd>(result);
	}
#pragma endregion
#pragma region Prefix
	template<typename T,int Rows,int Cols, int ResultCols>
	up<Eigen::Matrix<T,Rows,ResultCols>> EMatrix::Prefix( const Eigen::Matrix<T,Rows,Cols>& matrix, T value )
	{
		const auto rowCount = matrix.rows();
		const auto columnCount = matrix.cols();
		auto* result = new Eigen::Matrix<T,Rows,ResultCols>( rowCount, columnCount+1 );//a_1 = [ones(m, 1) X];
		//ForEach( matrix, [&result](size_t rowIndex, size_t columnIndex, double value)mutable{ )
		for( int64_t rowIndex=0; rowIndex<rowCount; ++rowIndex )
		{
			(*result)( rowIndex, 0 ) = value;
			for( int64_t colIndex=0; colIndex < columnCount; ++colIndex )
				(*result)( rowIndex, colIndex+1 ) = matrix( rowIndex, colIndex );
		}
		return up<Eigen::Matrix<T,Rows,ResultCols>>( result );
	}
#pragma endregion
#pragma region RandomIndexes
	template<int Rows, typename Scaler>
	up<Eigen::Matrix<Scaler,Rows,1>> EMatrix::RandomIndexes( Scaler upperBound, unsigned seed/*=0*/ )
	{
		const Scaler lowerBound = 0;
		std::vector<Scaler> randomIndexes(upperBound-lowerBound);
		//clog << "randomIndexes.size():  " << randomIndexes.size() << endl;
		for( Scaler i =lowerBound; i<upperBound; ++i )
			randomIndexes[i] = i;

		auto engine = seed==0 ? std::default_random_engine() : std::default_random_engine( seed );
		std::shuffle( randomIndexes.begin(), randomIndexes.end(), engine );

		auto results = new Eigen::Matrix<Scaler,Rows,1>( randomIndexes.size(), 1 );
		Scaler matrixIndex=0;
		for( const auto& randomIndex : randomIndexes )
		{
			(*results)(matrixIndex++) = randomIndex;
			//if( matrixIndex%100000==0 )
				//clog << "matrixIndex = " << matrixIndex <<  std::endl;
		}

		return up<Eigen::Matrix<Scaler,Rows,1>>(results);
	}
	//Set column value to the value of another random column.
	template<typename T,int Rows, int Cols>
	void EMatrix::SetRandomColumn( Matrix<T,Rows,Cols>& matrix, uint excludedColumnIndex, uint randomColumnIndex )noexcept
	{
		const auto columnCount = static_cast<uint>( matrix.cols() );
		THROW_IF( columnCount<4, "columnCount<4 {}", columnCount );
		if( randomColumnIndex==std::numeric_limits<uint>::max() )
			randomColumnIndex = columnCount-1;
		THROW_IF( randomColumnIndex>columnCount-1, "randomColumnIndex>columnCount-1 {}", randomColumnIndex );

		std::random_device rd;//https://stackoverflow.com/questions/686353/c-random-float-number-generation
		std::default_random_engine e2( rd() );
		std::uniform_real_distribution<> dist( 0, static_cast<double>(columnCount-2) );
		for( uint rowIndex=0; rowIndex<(uint)matrix.rows(); ++rowIndex )
		{
			auto columnIndex = static_cast<Eigen::Index>( std::floor(dist(e2)) );
			if( columnIndex>=(_int)excludedColumnIndex )
				++columnIndex;
			ASSERT( columnIndex<(_int)columnCount );
			matrix.coeffRef( rowIndex, randomColumnIndex ) = matrix.coeff( rowIndex, columnIndex );
		}
	}
#pragma endregion
#pragma region RemoveColumn/RemoveRow
	template<typename T>
	std::tuple<up<Eigen::SparseMatrix<T>>,up<Eigen::Matrix<T,-1,-1>>,std::vector<std::string>> EMatrix::RemoveDuplicates( const Eigen::SparseMatrix<T>& x, const Eigen::Matrix<T,-1,-1>& originalCorrelationCoeficient, const size_t offset, std::vector<std::string>& originalColumnNames, const T maxCorrelation )
	{
		std::unordered_set<size_t> duplicates;
		auto findDuplicates = [&duplicates, &offset, &originalColumnNames,&maxCorrelation](size_t rowIndex, size_t columnIndex, T value)mutable
		{
//			if( rowIndex<columnIndex &&  (value>maxCorrelation || value<-maxCorrelation) )
//			{
				//auto insertResult = duplicates.insert( columnIndex+offset );
				//if( insertResult.second )
					//clog << "removing row[" << columnIndex+offset <<"]=" << originalColumnNames[columnIndex+offset] << "duplcated with [" << rowIndex+offset << "]=" << originalColumnNames[rowIndex+offset] << std::endl;
//			}
		};
		ForEach<T,-1,-1>( originalCorrelationCoeficient, findDuplicates );

		Eigen::VectorXi columnCounts( x.cols()-duplicates.size() );
		int columnIndex = 0;
		size_t nonZeros=0;
		for( auto oldColumnIndex=0; oldColumnIndex<x.cols(); ++oldColumnIndex )
		{
			if( duplicates.find(oldColumnIndex)!=duplicates.end() )
				continue;
			int columnCount=0;
			for( typename Eigen::SparseMatrix<T>::InnerIterator it(x,oldColumnIndex); it; ++it )
				++columnCount;
			nonZeros+=columnCount;
			columnCounts(columnIndex++) = columnCount;
		}

		auto pResult = new Eigen::SparseMatrix<T>( x.rows(), x.cols()-int(duplicates.size()) );
		pResult->reserve( columnCounts );
		int columnOffset = 0;
		const size_t progressIndex = std::max( size_t(nonZeros/10), size_t(10000) );
		Stopwatch sw( "RemoveColumns", false );
		int index = 0;
		auto columnNames = originalColumnNames;
		auto pCorrelationCoeficient = new Eigen::Matrix<T,-1,-1>( originalCorrelationCoeficient );
		for( auto oldColumnIndex=0; oldColumnIndex<x.cols(); ++oldColumnIndex )
		{
			if( duplicates.find(oldColumnIndex)!=duplicates.end() )
			{
				columnNames.erase( columnNames.begin()+oldColumnIndex-columnOffset );
				RemoveColumn( *pCorrelationCoeficient, oldColumnIndex-columnOffset );
				RemoveRow( *pCorrelationCoeficient, oldColumnIndex-columnOffset );
				++columnOffset;
				continue;
			}
			for( typename Eigen::SparseMatrix<T>::InnerIterator it(x,oldColumnIndex); it; ++it )
			{
				const auto rowIndex = it.row();
				const T value = it.value();
				pResult->coeffRef( rowIndex, oldColumnIndex-columnOffset ) = value;
			}
			if( ++index%progressIndex==0 )
				sw.Progress( index, nonZeros );
		}
		return std::make_tuple( up<Eigen::SparseMatrix<T>>(pResult), up<Eigen::Matrix<T,-1,-1>>(pCorrelationCoeficient), columnNames );
	}
#pragma endregion
#pragma region RemoveColumn/RemoveRow
	template<typename T,int Rows, int Cols>
	void EMatrix::RemoveColumn( Eigen::Matrix<T,Rows,Cols>& matrix, size_t columnIndex )
	{

		const size_t rowCount = matrix.rows();
		const size_t columnCount = matrix.cols()-1;
		//clog << "rows:  " << matrix.rows() << std::endl;
		matrix.block( 0, columnIndex, rowCount, columnCount-columnIndex) = matrix.block( 0, columnIndex+1, rowCount, columnCount-columnIndex );

		matrix.conservativeResize(rowCount,columnCount);
		//clog << "rowCount:  " << matrix.rows() << ".  columnCount:  " << matrix.cols() << std::endl;
	}
	template<typename T>
	up<Eigen::SparseMatrix<T>> EMatrix::RemoveColumn( const Eigen::SparseMatrix<T>& sparse, int removedIndex )
	{
		const int rowCount = sparse.rows();
		const int columnCount = sparse.cols()-1;

		auto pColumnCounts = GetColumnCounts(sparse);

		SparseMatrix<T>* pResult = new SparseMatrix<T>( rowCount, columnCount );
		//clog << "rowCount:  " << rowCount << ".  columnCount:  " << columnCount << std::endl;
		if( pColumnCounts!=nullptr )
			RemoveRow<int,-1,1>( *pColumnCounts, removedIndex );
		else
		{
			pColumnCounts = up<Eigen::VectorXi>( new Eigen::VectorXi(Eigen::VectorXi::Zero(columnCount)) );
			auto function = [&pColumnCounts,&removedIndex](int rowIndex, int columnIndex, T value)mutable
			{
				if( columnIndex!=removedIndex && value!=T(0) )
					pColumnCounts->coeffRef( columnIndex>removedIndex ? columnIndex-1 : columnIndex )++;
			};
			ESparse::ForEachValue<T>( sparse, function );
		}
		pResult->reserve( *pColumnCounts );

		auto function = [&pResult,&removedIndex](int rowIndex, int columnIndex, T value)mutable
		{
			if( columnIndex!=removedIndex && value!=T(0) )
			{
				//clog << "rowIndex:  " << rowIndex << ".  columnIndex:  " << columnIndex << std::endl;
				pResult->coeffRef(rowIndex, columnIndex>removedIndex ? columnIndex-1 : columnIndex )=value;
			}
		};

		ESparse::ForEachValue<T>( sparse, function );

		return up<Eigen::SparseMatrix<T>>( pResult );
	}

	template<typename T,int Rows, int Cols>
	void EMatrix::RemoveRow( Eigen::Matrix<T,Rows,Cols>& matrix, size_t rowIndex )
	{
		const size_t rowCount = matrix.rows()-1;
		const size_t columnCount = matrix.cols();

		matrix.block( rowIndex, 0, rowCount-rowIndex, columnCount ) = matrix.block( rowIndex+1, 0, rowCount-rowIndex, columnCount );

		matrix.conservativeResize( rowCount, columnCount );
	}
#pragma endregion
#pragma region Sort
	template<typename T, int SortCount, int Rows, int Cols>
	up<Eigen::Matrix<T,Rows,Cols>> EMatrix::Sort(const Eigen::Matrix<T,Rows,Cols>& matrix, const std::array<int,SortCount>& columnIndexes, bool reverse/*=false*/, std::vector<size_t>* pOldRowIndexes/*=nullptr*/ )
	{
		std::set<std::array<T,SortCount+1>> sortedValues;
		for( int i=0; i<matrix.rows(); ++i )
		{
			std::array<T,SortCount+1> values;
			for( int j = 0; j<columnIndexes.size(); ++j )
				values[j] = matrix(i,j);
			values[SortCount] = T(i);
			sortedValues.insert( values );
		}
		auto pResult = new Matrix<T,Rows,Cols>( matrix.rows(), matrix.cols() );
		int i=0;
		auto pRow = sortedValues.begin();

		auto checkIterator = reverse ? sortedValues.begin() : sortedValues.end();
		for( auto pRow = reverse ? sortedValues.end() : sortedValues.begin(); pRow!=checkIterator; )
		{
			if( reverse )
				--pRow;
			const int oldRowIndex = int( pRow->at(SortCount) );
			if( pOldRowIndexes )
				pOldRowIndexes->push_back( oldRowIndex );
			for( auto j=0; j<matrix.cols(); ++j )
				pResult->coeffRef(i, j) = matrix.coeff( oldRowIndex, j );
			++i;
			if( !reverse )
				++pRow;
		}
		return up<Eigen::Matrix<T,Rows,Cols>>( pResult );
	}
	template<typename T, int Rows, int Cols>
	up<Eigen::Matrix<T,Rows,Cols>> EMatrix::Sort(const Eigen::Matrix<T,Rows,Cols>& matrix, const std::vector<size_t>& rowIndexes )
	{
		auto pResult = new Matrix<T,Rows,Cols>( matrix.rows(), matrix.cols() );
		int i=0;
		for( size_t oldRowIndex : rowIndexes )
		{
			for( int j=0; j<matrix.cols(); ++j )
				pResult->coeffRef(i,j) = matrix( oldRowIndex, j );
			++i;
		}
		return up<Eigen::Matrix<T,Rows,Cols>>(pResult);
	}

#pragma endregion
#pragma region StandardDeviationColumns

	template<typename T,int Rows,int Cols>
	EMatrix::RowVectorPtr<T,Cols> EMatrix::StandardDeviationColumns( const Eigen::Matrix<T,Rows,Cols>& matrix, bool sample/*=true*/, Eigen::Matrix<T,1,Cols>* pAverage/*=nullptr*/ )
	{
		RowVectorPtr<T,Cols> stdDev = VarianceColumns<T,Rows,Cols>( matrix, sample, pAverage );
		function<T(size_t,size_t,T)> squareRoot = [](size_t, size_t, T value){ return std::pow(value, T(.5)); };
		Transform<T,1,Cols>( *stdDev, squareRoot );

		return stdDev;
	}
	template<typename T>
	std::pair<up<Eigen::SparseVector<T>>,up<Eigen::SparseVector<T>>> EMatrix::StandardDeviationColumns( const SparseMatrix<T>& matrix, bool sample/*=true*/ )
	{
		std::pair<up<Eigen::SparseVector<T>>,up<Eigen::SparseVector<T>>> stdDevAverage = VarianceColumns<T>( matrix, sample );
		function<void(int,T&)> squareRoot = [](int, T& value){ value = std::pow(value, T(.5)); };
		TransformEachValue<T>( *stdDevAverage.first, squareRoot );

		return stdDevAverage;
	}
#pragma endregion
#pragma region VarianceColumns
	template<typename T,int Rows,int Cols>
	EMatrix::RowVectorPtr<T,Cols> EMatrix::VarianceColumns( const Eigen::Matrix<T,Rows,Cols>& matrix, bool sample/*=true*/, Eigen::Matrix<T,1,Cols>* pAverage/*=nullptr*/ )
	{
		const auto rowCount = matrix.rows();
		const auto columnCount = matrix.cols();

		auto centered = Centered(matrix, pAverage);
		const T denominator = T(sample ? rowCount-1 : rowCount);
		Eigen::Matrix<T,Cols,Cols> cov = (centered.adjoint() * centered) / denominator;
		RowVector<T,Cols>* pVariance = new RowVector<T,Cols>( columnCount );
		for( int64_t colIndex=0; colIndex < columnCount; ++colIndex )
			(*pVariance)(colIndex) = cov(colIndex,colIndex);

		return up<RowVector<T,Cols>>(pVariance);
	}
#pragma endregion
#pragma region SumColumns
	template<int Rows,int Cols>
	up<EMatrix::RowVectorD<Cols>> EMatrix::SumColumns( const MatrixD<Rows,Cols>& matrix )
	{
		RowVectorD<Cols>* result = new RowVectorD<Cols>( matrix.cols() );//::Zero( matrix.rows() );
		result->setConstant( 0.0 );
		auto sumFunction = [result](size_t,size_t columnIndex,double value)mutable{(*result)(columnIndex)+=value; };
		EMatrix::ForEach<double,Rows,Cols>( matrix, sumFunction );
		return up<RowVectorD<Cols>>( result );
	}
#pragma endregion
#pragma region SumRows
	template<typename T>
	up<Eigen::SparseVector<T>> EMatrix::SumRows( const SparseMatrix<T>& matrix )
	{
		Eigen::SparseVector<T>* pResult = new Eigen::SparseVector<T>( matrix.rows() );//::Zero( matrix.rows() );
		pResult->reserve( matrix.rows() );
		//EMatrix::ForEachValue<T>( matrix,  );
		//pResult->setConstant( 0.0 );
		auto sumFunction = [pResult](int rowIndex,int,T value)mutable
		{
			pResult->coeffRef( rowIndex )+=value;
		};
		ESparse::ForEachValue<T>( matrix, sumFunction );
		return up<Eigen::SparseVector<T>>( pResult );
	}
#pragma endregion
#pragma region ToDenseRowVector
	template<typename T>
	EMatrix::RowVectorPtr<T,-1> EMatrix::ToDenseRowVector( const Eigen::SparseVector<T>& vector )
	{
		auto pResult = new Eigen::Matrix<T,1,-1>( 1, vector.rows() );
		for( int rowIndex = 0; rowIndex<vector.rows(); ++rowIndex )
			pResult->coeffRef(0, rowIndex) = vector.coeff(rowIndex);

		return RowVectorPtr<T,-1>(pResult);
	}
#pragma endregion
#pragma region Select
	template<typename T,int64_t Rows,int64_t Cols>
	up<Eigen::Matrix<T,-1,Cols>> EMatrix::Select( const Eigen::Matrix<T,Rows,Cols>& matrix, const Eigen::Matrix<bool,Rows,1>& selection, bool notSelection/*=false*/ )
	{
		const auto rowCount = matrix.rows();
		size_t count = 0;
		if( notSelection )
			ForEach<bool,Rows>( selection, [&count](size_t rowIndex,bool value)mutable{ if( !value )++count; } );
		else
			ForEach<bool,Rows>( selection, [&count](size_t rowIndex, bool value)mutable{ if( value )++count; } );

		auto	pResult = new Eigen::Matrix<T,-1,Cols>(count, matrix.cols() );
		size_t index = 0;
		if( notSelection )
			ForEach<bool,Rows>( selection, [&matrix,pResult,&index](size_t rowIndex, bool value)mutable{ if( !value )pResult->row(index++)=matrix.row(rowIndex); } );
		else
			ForEach<bool,Rows>( selection, [&matrix,pResult,&index](size_t rowIndex, bool value)mutable{ if( value )pResult->row(index++)=matrix.row(rowIndex); } );
		return up<Eigen::Matrix<T,-1,Cols>>( pResult );
	}
	template<typename T>
	up<Eigen::SparseMatrix<T>> EMatrix::Select( const Eigen::SparseMatrix<T>& sparse, const int columnIndex, const std::function<bool(int i, const T& value)>& where )
	{
		auto columnCountsRowCount = ESparse::GetColumnCounts( sparse, columnIndex, where );
		const auto& rowIndexes = columnCountsRowCount.second;
		auto pMatrix = new Eigen::SparseMatrix<T>( int(rowIndexes.size()), sparse.cols() );
		auto pColumnCounts = std::move( columnCountsRowCount.first );
		pMatrix->reserve( *pColumnCounts );
		auto func = [&rowIndexes, &pMatrix, &sparse](int i, int j, T rowResult)mutable
		{
			auto pOldNewIndex = rowIndexes.find(i);
			if( pOldNewIndex!=rowIndexes.end() )
				pMatrix->insert( pOldNewIndex->second, j ) = sparse.coeff(i,j);
		};
		ESparse::ForEachValue<T>( sparse, func );
		return up<Eigen::SparseMatrix<T>>(pMatrix);
	}

	template<typename T>
	up<Eigen::SparseMatrix<T>> EMatrix::Select( const Eigen::SparseMatrix<T>& sparse, const std::vector<int>& columnIndexes )
	{
		auto pAllColumnCounts = GetColumnCounts( sparse, &columnIndexes );
		auto pResult = new Eigen::SparseMatrix<T>( sparse.rows(), int(columnIndexes.size()) );
		pResult->reserve( *pAllColumnCounts );
		std::function<void(int,int,T)> function = [&pResult](int i, int j, T value)mutable
		{
			//if( j==1 )
				//clog << "[" << i << ", " << j << "]=" << value << std::endl;
			pResult->insert(i, j) = value;
		};
		ESparse::ForEachValue<T>( sparse, function, &columnIndexes );
		return SparseTPtr<T>( pResult );
	}

#pragma endregion
#pragma region TopVale
	template<typename T>
	std::pair<up<Eigen::SparseVector<T>>,T> EMatrix::TopValue( const Eigen::SparseVector<T>& vector, size_t count )
	{
		std::multimap<T,int> topValues;
		auto function = [&topValues,&count](int rowIndex, const T* pValue)
		{
			auto value = pValue ? *pValue : T(0);
			if( topValues.size()<count )
				topValues.insert( std::make_pair(value, rowIndex) );
			else
			{
				T cutoff = topValues.begin()->first;
				if( value>=cutoff )
				{
					topValues.insert( std::make_pair(value, rowIndex) );
					if( value>cutoff )
						topValues.erase( topValues.begin() );
				}
			}
		};
		ESparse::ForEach<T>( vector, function );
		auto pResult = CreateSparseVector( vector );
		for( const auto& valueIndex : topValues )
		{
			pResult->coeffRef(valueIndex.second)=1.0;
		}
		return std::make_pair( std::move(pResult), topValues.size()==0 ? std::numeric_limits<T>::min() : topValues.begin()->first );
	}
#pragma endregion
#pragma region Transform
	template<typename T,int64_t Rows,int64_t Cols>
	void EMatrix::Transform( Eigen::Matrix<T,Rows,Cols>& matrix, const std::function<void(size_t,size_t,T&)>& function )
	{
		const auto rowCount = matrix.rows();
		const auto size = matrix.size();
		const bool isRowMajor = matrix.IsRowMajor;
		for( int64_t i = 0; i < size; ++i )
		{
			auto rowIndex = isRowMajor ? i/rowCount : i%rowCount;
			auto columnIndex = isRowMajor ? i%rowCount : i/rowCount;
			function( rowIndex, columnIndex, *(matrix.data() + i) );
		}
	}
#pragma endregion
#pragma region ForEachRowColumn
	template<typename T,int64_t Rows,int64_t Cols>
	void EMatrix::ForEachRowColumn( const Eigen::Matrix<T,Rows,Cols,0>& matrix, uint columnIndex, function<void(uint rowIndex,const T& value)> func )
	{
		const uint rowCount = matrix.rows();
		const T* pStart = matrix.data()+columnIndex*rowCount;
		for( uint rowIndex=0; rowIndex<rowCount; ++rowIndex, ++pStart )
			func( rowIndex, *pStart );
	}
#pragma endregion
#pragma region RemoveRows
	template<typename T,int64_t Rows,int64_t Cols>
	EMatrix::MPtr<T,Rows,Cols> EMatrix::RemoveRows( const Eigen::Matrix<T,Rows,Cols,0>& matrix, const flat_set<uint>& indexes )noexcept
	{
		const auto rowCount = matrix.rows();
		auto pResults = make_unique<Eigen::Matrix<T,Rows,Cols,0>>( rowCount-indexes.size(), matrix.cols() );
		const uint size = matrix.size();
		uint skippedRows = 0;
		uint lastColumnIndex = std::numeric_limits<uint>::max();
		for( uint i = 0; i < size; ++i )
		{
			const auto columnIndex = i/rowCount;
			if( columnIndex!=lastColumnIndex )
			{
				skippedRows = 0;
				lastColumnIndex = columnIndex;
			}
			auto rowIndex = i%rowCount;
			if( indexes.find(rowIndex)!=indexes.end() )
				++skippedRows;
			else
				pResults->coeffRef( rowIndex-skippedRows, columnIndex) = *(matrix.data() + i);
		}
		return pResults;
	}
#pragma endregion
#pragma region ExtractColumns
	template<typename T,int Rows,int Cols,int ResultCols>
	EMatrix::MPtr<T,Rows,ResultCols> EMatrix::ExtractColumns( const Eigen::Matrix<T,Rows,Cols,0>& matrix, const vector<uint>& indexes )noexcept
	{
		static_assert( Rows!=1, "Use ExtractColumnsRV" );
		up<Matrix<T,Rows,ResultCols>> pResults = make_unique<Matrix<T,Rows,ResultCols>>( matrix.rows(), indexes.size() );

		//todo just copy whole columns from one matrix to another.
		const uint rowCount = matrix.rows();
		const uint colCount = pResults->cols();
		const uint originalColumnCount = matrix.cols();
		uint toColumnIndex = 0;
		for( const auto columnIndex : indexes )
		{
			for( uint rowIndex=0; rowIndex<rowCount; ++rowIndex )
			{
				ASSERT( columnIndex<originalColumnCount );
				T value = *(matrix.data() + columnIndex*rowCount+rowIndex);
				ASSERT( toColumnIndex<colCount );
				pResults->coeffRef(rowIndex,toColumnIndex) = value;
			}
			++toColumnIndex;
		}

		return pResults;
	}
	template<typename T>
	up<Eigen::Matrix<T,1,-1,1>> EMatrix::ExtractColumnsRV( const Eigen::Matrix<T,1,-1>& matrix, const vector<uint>& indexes )noexcept
	{
		auto pResults = make_unique<Matrix<T,1,-1,1>>( 1, indexes.size() );
		uint toColumnIndex = 0;
		for( const auto columnIndex : indexes )
		{
			T value = *(matrix.data() + columnIndex);
			pResults->coeffRef(0,toColumnIndex++) = value;
		}
		return pResults;
	}
#pragma endregion

#pragma warning( pop )
}
#undef var