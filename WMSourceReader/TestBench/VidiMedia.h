#ifndef _VIDIATOR_MEDIA_H_
#define _VIDIATOR_MEDIA_H_


HRESULT GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin, WCHAR *pPinName);


//!! this object should always be created with "new" !!
class VidiMedia: public IMediaSample
{
public:
	VidiMedia()
	{
		RefCounter = 1;
		mt = NULL;
		Buff = NULL;
		Size = 0;
	}
	~VidiMedia()
	{
		ASSERT( RefCounter == 0 );
		if( Buff )
		{
			_aligned_free( Buff );
			Buff = NULL;
		}
	}
	HRESULT STDMETHODCALLTYPE GetPointer( __out  BYTE **ppBuffer) 
	{ 
		if( ppBuffer )
			*ppBuffer = Buff; 
		return S_OK;
	}

	long STDMETHODCALLTYPE GetSize( void) 
	{ 
		return Size; 
	}
	      
	long STDMETHODCALLTYPE GetActualDataLength( void) 
	{
		return Size;
	}

	HRESULT STDMETHODCALLTYPE SetActualDataLengthBlocking( long __MIDL__IMediaSample0000)
	{
		if( Size == __MIDL__IMediaSample0000 )
			return S_OK;
		//block realloc until only 1 thread is accesing the pointer
		while( RefCounter > 1 )
			Sleep( 1 );
		Size = __MIDL__IMediaSample0000;
		if( Buff )
			_aligned_free( Buff );
		Buff = (BYTE *)_aligned_malloc( Size, 32 );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE SetActualDataLengthCanFail( long __MIDL__IMediaSample0000)
	{
		if( Size == __MIDL__IMediaSample0000 )
			return S_OK;
		//more then 1 thread is using us. It is not safe to resize buffer atm
		if( RefCounter > 1 )
		{
			//not correct. But might save us from a read / write corruption if we did not test the resize result
			if( Size >= __MIDL__IMediaSample0000 )
				return S_OK;
			//try to make sure you initialize this buffer at the beggining of the code
			return S_FALSE;
		}
		Size = __MIDL__IMediaSample0000;
		if( Buff )
			_aligned_free( Buff );
		Buff = (BYTE *)_aligned_malloc( Size, 32 );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE SetActualDataLength( long __MIDL__IMediaSample0000)
	{
		if( Size == __MIDL__IMediaSample0000 )
			return S_OK;
		Size = __MIDL__IMediaSample0000;
		if( Buff )
			_aligned_free( Buff );
		Buff = (BYTE *)_aligned_malloc( Size, 32 );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetMediaType( __out  AM_MEDIA_TYPE **ppMediaType)
	{
		if( ppMediaType )
			*ppMediaType = CreateMediaType( mt );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE SetMediaType( __in  AM_MEDIA_TYPE *pMediaType)
	{
		mt = pMediaType;
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef( void)
	{ 
		RefCounter++;
		return RefCounter;
	}
	ULONG STDMETHODCALLTYPE Release( void)
	{ 
		RefCounter--;
		if( RefCounter == 0 )
		{
			delete this;
			return 0;
		}
		return RefCounter;
	}

	HRESULT STDMETHODCALLTYPE GetTime( 
		__out  REFERENCE_TIME *pTimeStart,
		__out  REFERENCE_TIME *pTimeEnd) 
	{ 
		if( pTimeStart )
			*pTimeStart = 1; 
		if( pTimeEnd )
			*pTimeEnd = 2; 
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE SetTime( 
		__in_opt  REFERENCE_TIME *pTimeStart,
		__in_opt  REFERENCE_TIME *pTimeEnd){ return E_NOTIMPL;};

	HRESULT STDMETHODCALLTYPE IsSyncPoint( void){ return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE SetSyncPoint( BOOL bIsSyncPoint){ return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE IsPreroll( void){ return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE SetPreroll( BOOL bIsPreroll){ return E_NOTIMPL; };
	HRESULT STDMETHODCALLTYPE IsDiscontinuity( void){ return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE SetDiscontinuity(BOOL bDiscontinuity){return E_NOTIMPL;};
	HRESULT STDMETHODCALLTYPE GetMediaTime( 
		__out  LONGLONG *pTimeStart,
		__out  LONGLONG *pTimeEnd){return E_NOTIMPL;};

	HRESULT STDMETHODCALLTYPE SetMediaTime( 
		__in_opt  LONGLONG *pTimeStart,
		__in_opt  LONGLONG *pTimeEnd){return E_NOTIMPL;};
	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,
			__RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject){return E_NOTIMPL;}
private:
	AM_MEDIA_TYPE	*mt;
	BYTE			*Buff;
	int				Size;
	long			RefCounter;
};

#endif
