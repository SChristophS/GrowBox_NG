// components/ProtectedRoute.js
import React from "react";
import { Route, Navigate } from "react-router-dom";

const ProtectedRoute = ({ element: Element, ...rest }) => {
  const isAuthenticated = localStorage.getItem("user_id") !== null;

  return (
    <Route
      {...rest}
      element={
        isAuthenticated ? (
          <Element />
        ) : (
          <Navigate
            to={{
              pathname: "/login",
              state: { from: rest.path },
            }}
          />
        )
      }
    />
  );
};

export default ProtectedRoute;
