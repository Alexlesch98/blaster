#include <Rcpp.h>
#include <string>
#include <memory>

#include "FASTA/Reader.h"
#include "Alphabet/DNA.h"
#include "FileFormat.h"
#include "Database.h"
#include "Database/HitWriter.h"
#include "Database/GlobalSearch.h"
#include "Sequence.h"
#include "Alphabet/DNA.h"
#include "Alphabet/Protein.h"

#include "Common.h"
#include "FileFormat.h"
#include "WorkerQueue.h"

using namespace Rcpp;


template < typename A >
using QueryWithHits     = std::pair< Sequence< A >, HitList< A > >;

template < typename A >
using QueryWithHitsList = std::deque< QueryWithHits< A > >;

template < typename A >
class QueueItemInfo< QueryWithHitsList< A > > {
public:
  static size_t Count( const QueryWithHitsList< A >& list ) {
    return std::accumulate(
                           list.begin(), list.end(), 0,
                           []( int sum, const QueryWithHits< A >& q ) { return sum + q.second.size(); } );
  }
};

template < typename A >
class SearchResultsWriterWorker {
public:
  SearchResultsWriterWorker( const std::string& path )
    : mWriter( std::move(
                         DetectFileFormatAndOpenHitWriter< A >( path, FileFormat::ALNOUT ) ) ) {}

  void Process( const QueryWithHitsList< A >& queryWithHitsList ) {
    for( auto& queryWithHits : queryWithHitsList ) {
      ( *mWriter ) << queryWithHits;
    }
  }

private:
  std::unique_ptr< HitWriter< A > > mWriter;
};

template < typename A >
using SearchResultsWriter =
  WorkerQueue< SearchResultsWriterWorker< A >, QueryWithHitsList< A >,
               const std::string& >;

template < typename A >
class QueueItemInfo< SequenceList< A > > {
public:
  static size_t Count( const SequenceList< A >& list ) {
    return list.size();
  }
};

template < typename A >
class QueryDatabaseSearcherWorker {
public:
  QueryDatabaseSearcherWorker( SearchResultsWriter< A >* writer,
                               const Database< A >*      database,
                               const SearchParams< A > &params )
    : mWriter( *writer ),
      mGlobalSearch( *database, params ) {}

  void Process( const SequenceList< A >& queries ) {
    QueryWithHitsList< A > list;

    for( auto& query : queries ) {
      auto hits = mGlobalSearch.Query( query );
      if( hits.empty() )
        continue;

      list.push_back( { query, hits } );
    }

    if( !list.empty() ) {
      mWriter.Enqueue( list );
    }
  }

private:
  GlobalSearch< A >         mGlobalSearch;
  SearchResultsWriter< A >& mWriter;
};

template < typename A >
using QueryDatabaseSearcher =
  WorkerQueue< QueryDatabaseSearcherWorker< A >, SequenceList< A >,
               SearchResultsWriter< A >*, const Database< A >*,
               const SearchParams< A >& >;

template < typename A >
struct WordSize {
  static const int VALUE = 8; // DNA, default
};

template <>
struct WordSize< Protein > {
  static const int VALUE = 5;
};

std::string DFtoSeq(DataFrame seq_table)
{
  std::vector< std::string > ids = seq_table["Id"];
  std::vector< std::string > seqs = seq_table["Seq"];
  
  std::stringstream content;

  for (int i{0}; i < ids.size(); ++i) {
    std::string id{ids[i]};
    std::string seq{seqs[i]};
    content << ">" << id << "\n" << seq << "\n";
  }
  return content.str();
}

// [[Rcpp::export]]

void blast(DataFrame query_table,
           DataFrame db_table,
           std::string output_file,
           int maxAccepts = 1,
           int maxRejects =  16,
           float minIdentity = 0.75) 
{

  std::istringstream db_stream( DFtoSeq(db_table) );
  std::unique_ptr< SequenceReader< DNA > > dbReader( new FASTA::Reader< DNA >( db_stream ) );
  
  Sequence< DNA > seq;
  SequenceList< DNA > sequences;

  while( !(dbReader->EndOfFile()) ) {
    (*dbReader) >> seq;
    sequences.push_back( std::move( seq ) );
  }

  ProgressOutput progress;

  enum ProgressType {
                     ReadDBFile,
                     StatsDB,
                     IndexDB,
                     ReadQueryFile,
                     SearchDB,
                     WriteHits
  };

  progress.Add( ProgressType::ReadDBFile, "Read database", UnitType::BYTES );
  progress.Add( ProgressType::StatsDB, "Analyze database" );
  progress.Add( ProgressType::IndexDB, "Index database" );
  progress.Add( ProgressType::ReadQueryFile, "Read queries", UnitType::BYTES );
  progress.Add( ProgressType::SearchDB, "Search database" );
  progress.Add( ProgressType::WriteHits, "Write hits" );

  // Read DB
  progress.Activate( ProgressType::ReadDBFile );
  while( !dbReader->EndOfFile() ) {
    (*dbReader) >> seq;
    sequences.push_back( std::move( seq ) );
    progress.Set( ProgressType::ReadDBFile, dbReader->NumBytesRead(),
                  dbReader->NumBytesTotal() );
  }

  // Index DB
  Database< DNA > db( WordSize< DNA >::VALUE );
  db.SetProgressCallback(
                         [&]( typename Database< DNA >::ProgressType type, size_t num, size_t total ) {
                           switch( type ) {
                           case Database< DNA >::ProgressType::StatsCollection:
                             progress.Activate( ProgressType::StatsDB )
                               .Set( ProgressType::StatsDB, num, total );
                             break;

                           case Database< DNA >::ProgressType::Indexing:
                             progress.Activate( ProgressType::IndexDB )
                               .Set( ProgressType::IndexDB, num, total );
                             break;

                           default:
                             break;
                           }
                         } );
  db.Initialize( sequences );

  // Read and process queries
  const int numQueriesPerWorkItem = 64;
  
  SearchParams< DNA > searchParams;

  searchParams.maxAccepts = maxAccepts;
  searchParams.maxRejects = maxRejects;
  searchParams.minIdentity = minIdentity;

  SearchResultsWriter< DNA >   writer( 1, output_file );
  QueryDatabaseSearcher< DNA > searcher( -1, &writer, &db, searchParams );

  searcher.OnProcessed( [&]( size_t numProcessed, size_t numEnqueued ) {
                          progress.Set( ProgressType::SearchDB, numProcessed, numEnqueued );
                        } );
  writer.OnProcessed( [&]( size_t numProcessed, size_t numEnqueued ) {
                        progress.Set( ProgressType::WriteHits, numProcessed, numEnqueued );
                      } );

  std::istringstream query_stream( DFtoSeq(query_table) );
  std::unique_ptr< SequenceReader< DNA > > qryReader( new FASTA::Reader< DNA >( query_stream ) );

  SequenceList< DNA > queries;
  progress.Activate( ProgressType::ReadQueryFile );
  while( !qryReader->EndOfFile() ) {
    qryReader->Read( numQueriesPerWorkItem, &queries );
    searcher.Enqueue( queries );
    progress.Set( ProgressType::ReadQueryFile, qryReader->NumBytesRead(),
                  qryReader->NumBytesTotal() );
  }

  // Search
  progress.Activate( ProgressType::SearchDB );
  searcher.WaitTillDone();

  progress.Activate( ProgressType::WriteHits );
  writer.WaitTillDone();

  Rcout << "\n";
}

