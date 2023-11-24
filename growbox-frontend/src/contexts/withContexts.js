// withContexts.js
import React from 'react';
import { AuthContext } from '../contexts/AuthContext';
import { GrowPlanContext } from '../contexts/GrowPlanContext';

const withContexts = WrappedComponent => {
  return class extends React.Component {
    render() {
      return (
        <AuthContext.Consumer>
          {authContext => (
            <GrowPlanContext.Consumer>
              {growPlanContext => (
                <WrappedComponent
                  {...this.props}
                  authContext={authContext}
                  growPlanContext={growPlanContext}
                />
              )}
            </GrowPlanContext.Consumer>
          )}
        </AuthContext.Consumer>
      );
    }
  };
};

export default withContexts;
