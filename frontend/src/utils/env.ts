export const getApiBaseUrl = (): string => {
  return import.meta.env.VITE_API_BASE_URL || ''
}

export const getWsBaseUrl = (): string => {
  return import.meta.env.VITE_WS_BASE_URL || ''
}

export const getApiUrl = (path: string): string => {
  // In development, use relative URLs that will be proxied by Vite
  // In production, use the configured base URL or relative URLs
  const baseUrl = getApiBaseUrl()
  const cleanPath = path.startsWith('/') ? path : `/${path}`

  // if (baseUrl) 
  {
    const _ret = `${baseUrl}${cleanPath}`
    // console.log('getApiUrl', _ret)
    return _ret
  }

  // Use relative URLs when no base URL is configured (development mode)
  // return cleanPath
}

export const getWsUrl = (path: string): string => {
  // In development, use relative URLs that will be proxied by Vite
  // In production, use the configured base URL or construct WebSocket URL
  const baseUrl = getWsBaseUrl()
  const cleanPath = path.startsWith('/') ? path : `/${path}`

  // if (baseUrl) 
  {
    const _ret = `${baseUrl}${cleanPath}`
    // console.log('getApiUrl', _ret)
    return _ret
  }

  // Use relative WebSocket URLs when no base URL is configured (development mode)
  // Convert http/https to ws/wss for WebSocket connections
  // const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
  // return `${protocol}//${window.location.host}${cleanPath}`
}