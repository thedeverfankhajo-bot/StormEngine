#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, StormEngineLifecycleState) {
    StormEngineLifecycleStateCreated = 0,
    StormEngineLifecycleStateForegroundInactive,
    StormEngineLifecycleStateForegroundActive,
    StormEngineLifecycleStateBackground,
    StormEngineLifecycleStateTerminating,
};

// UIKit adapter for one UIWindowScene lifecycle stream.
// Configure this class (or a subclass) in the host application's
// UISceneConfiguration. Each instance owns independent lifecycle state.
// Available from iOS 13.0. All callbacks run on the UIKit main thread.
@interface StormEngineSceneDelegate : UIResponder <UIWindowSceneDelegate> {
@private
    void *_stormEngineLifecycleStorage;
}

@property(nonatomic, readonly) StormEngineLifecycleState stormLifecycleState;

// Override to forward normalized scene state into the host engine runtime.
// This callback is synchronous on the main thread and must not block.
- (void)stormScene:(UIScene *)scene
    didTransitionToState:(StormEngineLifecycleState)state;

// App-scoped, best-effort termination hook. Persistent state must be saved
// incrementally and must not depend on this callback.
- (void)stormApplicationWillTerminate;

// Override to integrate host diagnostics. The default implementation logs
// only the error code and fixed adapter message, never project or user data.
- (void)stormLifecycleDidFailWithCode:(NSInteger)code
                              message:(NSString *)message;

@end

NS_ASSUME_NONNULL_END
