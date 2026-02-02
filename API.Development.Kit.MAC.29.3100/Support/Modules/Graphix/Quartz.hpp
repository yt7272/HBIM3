#include "GraphixDefinitions.hpp"
#include <ApplicationServices/ApplicationServices.h>
#include "IBlit.hpp"


namespace NewDisplay {

	class	INativeContextImpl;
	class	NativeImage;

};


namespace Quartz {

	class GRAPHIX_DLL_EXPORT QuartzBlitTarget: public NewDisplay::BlitTarget
	{
	private:
		CGContextRef	context;
		USize			width;
		USize			height;
		double			resolutionFactor;

	public:
		QuartzBlitTarget (CGContextRef context, USize width, USize height, double resolutionFactor,
						 NewDisplay::NativeContextImpl* dstContext = nullptr);
		virtual ~QuartzBlitTarget ();

		CGContextRef	GetContext ();
		USize			GetWidth () const;
		USize			GetHeight () const;
		double			GetResolutionFactor () const;
	};	
	
	GRAPHIX_DLL_EXPORT	NewDisplay::INativeContextImpl*		CreateQuartzContext (CGContextRef cgContextRef, USize width, USize height, double resolutionFactor);
	
	GRAPHIX_DLL_EXPORT	CGImageRef							GetNativeImageData (const NewDisplay::NativeImage& image);
	GRAPHIX_DLL_EXPORT	void								ReleaseNativeImageData (const NewDisplay::NativeImage& image, CGImageRef nativeImageData);
	
};
