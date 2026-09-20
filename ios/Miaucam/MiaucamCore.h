#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface MiaucamCore : NSObject

+ (void)reset;

+ (int)advanceWithBGRA:(const uint8_t *)pixels
                 width:(int)width
                height:(int)height
                stride:(int)stride
           timestampMs:(double)timestampMs
                 hands:(NSData *)hands
                  face:(NSData *)face
       blendshapeNames:(NSArray<NSString *> *)names
      blendshapeScores:(NSData *)scores
             transform:(NSData *)transform;

+ (NSArray<NSString *> *)memeFilesForGesture:(int)gesture;
+ (BOOL)isVideoGesture:(int)gesture;

@end

NS_ASSUME_NONNULL_END
